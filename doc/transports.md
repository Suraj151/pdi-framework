# 10. Transports

A transport speaks a wire protocol on a byte stream. It sits between the `iClientInterface` the device port provides and the service that deals in payloads. All three — HTTP, MQTT, SMTP — share the same shape: take a client, speak the protocol on it, hand the parsed result back. None of them knows about the device, owns a schedule, or keeps global state beyond its own buffers.

```
   service          knows what to do, when to retry, what to persist
      │  hands over an iClientInterface* it owns
      ▼
   transport        knows how to speak the protocol; parses bytes, nothing else
      │
      ▼
   iClientInterface  plaintext TCP or TLS — the transport can't tell
```

That split buys three things. The same HTTP parsing serves both the client and the portal. A port that ships an SDK-native MQTT client can present it as an `iClientInterface` and the framework's own client falls away without any service noticing. And because a transport is plain C++ over a stream, it can be exercised on a host with no radio involved.

Live in [src/transports/](../src/transports/). There are no globals — each consumer creates an instance.

| Transport | Class | Used by | Speaks |
|---|---|---|---|
| HTTP | `Http_Client` | OTA, IoT, GPIO posting, `wget`, and the portal indirectly | HTTP/1.1, and 1.0 |
| MQTT | `MQTTClient` plus the message builders | MQTT service, IoT service | MQTT 3.1.1 |
| SMTP | `SMTPClient` | email service | SMTP with AUTH LOGIN |

## 10.1 HTTP

Two POD records carry request and response metadata; the class is the state machine between them.

```cpp
Http_Client http;
http.Begin();
http.SetClient(__i_instance.getNewTcpClientInstance());
http.SetTimeout(HTTP_REQUEST_DURATION);   // 10 s
http.SetKeepAlive(true);
http.SetFollowRedirects(true);
http.SetDefaultHeaders(true);             // Host, User-Agent, Connection
http.AddReqHeader("X-Device", mac);

int16_t code = http.Get("http://api.example.com/v1/ping");

char *body; int16_t len;
http.GetResponse(body, len);              // borrowed buffer — copy if you need it later
http.End(/*preserve_client=*/true);
```

`Get`, `Post` and `DownloadStream` are synchronous: they return once the reply is in. A negative return means the transport failed before there was an HTTP status to report — connect, read or timeout.

`GetAsync(url, on_complete)` takes a callback instead. Where the board can run work beside the main loop the request goes out there; otherwise it runs inline, the way `Get` does. Either way `on_complete` is called from the main loop, with the client, once the response is in.

A reply is read until it is complete, not until the line goes quiet. A reply that carries `Content-Length` ends as soon as that many bytes are in, and so does one the server closes after sending. Only a reply with no length, on a connection the server keeps open, waits out the 1.5 s of silence below. `DownloadStream` hands on a body only as a whole: one shorter than its `Content-Length`, or a chunked one that ends before its last chunk (chunk sizes are read to 32 bits, extensions after `;` ignored), returns `HTTP_ERROR_BODY_INCOMPLETE`. A reply read into memory keeps what fits in the response buffer and logs the cut.

| Setting | Default | Meaning |
|---|---|---|
| port | 80, or 443 for `https://` | used when the URL omits one |
| version | HTTP/1.1 | request line |
| keep-alive | 30 s | what is advertised to the peer |
| request budget | 10 s | whole request |
| working buffer | 640 B | used during parsing |
| silence before giving up on a read | 1.5 s | `HTTP_CLIENT_MAX_READ_MS`; a reply with a length, or a closed connection, ends sooner |
| connect retries | 1 | everything above this belongs to the service |

**HTTPS is the same client on a different socket.** The class never opens the connection, so making a request encrypted is a change of factory call and nothing else:

```cpp
iTlsClientInterface* tls = __i_instance.getNewTlsClientInstance();
tls->setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);
tls->setSNIHostname("api.example.com");
http.SetClient(tls);

int16_t code = http.Get("https://api.example.com/v1/ping");
```

Handshake, verification, record framing and the same response parsing all come along. `setVerifyPeer(false)` keeps the channel encrypted while skipping chain validation, which is right for a self-signed development box and wrong for anything crossing a network you don't own.

**The URL picks the client.** Each request reads its URL first and takes the client that URL needs, changing the one it holds only if it is the wrong sort. An `http://` URL goes over the plain client, an `https://` URL over the TLS one — and that includes a redirect that switches from one to the other half way, which you could not have known about when you started the request. Handing in your own client with `SetClient()` still works, and it is left alone as long as it matches.

**`https://` needs a board that can do TLS and a build with it switched on.** Without both there is no secure client to use, and the request fails with an error rather than going out unencrypted. `http://` works on any build with a network.

Because the choice is made per request, OTA, device-IoT and GPIO posting all share one client and none of them has to pick anything; with TLS on, a plain `http://` address still goes out as plain HTTP.

**One shared client means one request at a time.** `Http_Client::GetStaticInstance()` is that client, and mail goes out over the same socket. While an async request is still waiting for its callback, `Begin()` returns false and `DownloadStream` returns `PDI_ERR_BUSY`, so nothing takes the socket out from under a request in flight. The built-in users take their turn: a GPIO alert raised while the client is busy goes out on the next tick, a mail is not started until the client is free and its send reports failure, and `wget` answers "http client busy, try again".

There is no HTTP *server* class here. The server side lives at the interface layer with a portable default implementation, plumbed through the web server, and `begin(port, secure)` is what flips it into TLS ([§8.7.1](web-server.md#871-https-wiring-and-certificates)).

## 10.2 MQTT

Split in two by concern: a pure encoder and decoder that turns operations into wire records with no I/O at all, and the client class that owns the connection state machine — handshake, keep-alive, subscriptions, QoS acknowledgement, callback dispatch.

```cpp
MQTTClient mqtt;
mqtt.begin(client, &generalCfg, &lwtCfg);
mqtt.OnConnected(&onMqttConnected);
mqtt.OnData(&onMqttData);              // (args, topic, topic_len, data, len)
mqtt.OnDisconnected(&onMqttDisconnected);

mqtt.InitConnection(host, MQTT_DEFAULT_PORT, /*security=*/0);
mqtt.InitClient(clientId, user, pass, /*keepAlive=*/60, /*cleanSession=*/1);
mqtt.InitLWT(willTopic, willMsg, /*qos=*/1, /*retain=*/0);

mqtt.Connect();
mqtt.Subscribe("ctrl/+/cmd", /*qos=*/1);
mqtt.Publish("sensor/temp", payload, len, /*qos=*/1, /*retain=*/0);
```

`Subscribe` reports success when the packet is enqueued, not when the broker acknowledges it — wait for the subscribed callback before publishing on a topic you just asked for.

Callbacks fire on whichever lane drives the MQTT service, which by default is the inline scheduler. Don't block inside one; schedule the expensive part as a follow-up tick, the way the IoT service does.

The encoder is usable on its own when you need a packet without owning a connection — bind a buffer, build a connect or publish record, and write its bytes to whatever stream you have.

Defaults are port 1883 and a 30-second keep-alive. The service translates its three config tables into the `Init*` calls above.

Every tick first reads what the broker has sent, up to `MQTT_RECV_PACKETS_PER_TASK` packets, and then sends from the queue, up to `MQTT_SEND_PACKETS_PER_TASK`. Up to `MQTT_INFLIGHT_MAX` qos 1 publishes (4, 8 on boards with the RAM) go out before their acknowledgements come back; a subscribe, an unsubscribe, a qos 2 publish or a ping waits for its answer before the next packet. A qos 1 publish left unacknowledged for `MQTT_READ_TIMEOUT_MS` makes the client reconnect. A burst in either direction is taken a few packets per tick, and a message from the broker is never left waiting behind the device's own queue. Both default to 3 in [src/config/MqttConfig.h](../src/config/MqttConfig.h).

Each queued packet takes its own piece of heap, and the queue holds at most `MQTT_SEND_QUEUE_MAX` bytes; past that a publish is refused and returns false, and nothing already queued is dropped. A publish bigger than the client's standing buffer is built in a buffer of its own exact size, up to `MQTT_PACKET_MAX`. When the broker's answer to a packet arrives, the next queued packet goes out in the same pass. When the connection cannot take a packet yet, the client waits up to `MQTT_SEND_ROOM_WAIT_MS` (200) for room and otherwise leaves it queued for the next pass; only a connection that takes nothing for `MQTT_SEND_TIMEOUT_MS` (5 s) is given up and reconnected. A message from the broker larger than the standing buffer, up to `MQTT_PACKET_MAX`, is read into a buffer of its own size and delivered whole. One larger than that, or one the heap has no room for, is read past and dropped with a syslog warning. A packet that stops arriving part way leaves the stream out of step, so it is never delivered: the connection is dropped and the client connects afresh.

The tick itself runs every `MQTT_TIMER_INTERVAL_MS`, 200 by default. While a message waits to go or an answer from the broker is due, the same task runs every `MQTT_DRAIN_INTERVAL_MS`, 10 by default, and goes back to its normal pace once nothing is pending. A shorter interval makes the client read and send sooner, at the cost of more of the loop's time while there is traffic; 1000 gives a one-second pace for a board that needs the time elsewhere. The client's own timeouts are set in milliseconds beside it (`MQTT_HOST_CONNECT_TIMEOUT_MS`, `MQTT_READ_TIMEOUT_MS`) and counted in ticks rounded up, so they last as long at any interval. `MQTT_CONNECT_WAIT_MS`, 2500 by default, is how long one connect attempt to the broker may hold the loop. The keep-alive stays in seconds, because that is how the protocol carries it. A keep-alive ping that gets no answer within `MQTT_READ_TIMEOUT_MS` makes the client reconnect, while queued messages keep going out during the wait.

## 10.3 SMTP

A blocking, command-response client. Each helper issues one verb and waits for the reply code it expects.

```cpp
SMTPClient smtp;
smtp.begin(client, host, port);        // stores parameters; connects on first send

smtp.sendHello(domain);
smtp.sendAuthLogin(username, password);
smtp.sendFrom(sender);
smtp.sendTo(recipient);                // once per recipient
smtp.sendDataCommand();
smtp.sendDataHeader(sender, recipient, subject);
smtp.sendDataBody(body);
smtp.sendQuit();
smtp.end();
```

Lower-level primitives are exposed too — read a response, wait for an expected one, send a command and get its code — for callers that want to drive the protocol themselves.

Every verb blocks on the wire with a five-second timeout, so a flaky link can add up across a single message. That is why the email service schedules sending off the critical path rather than calling it from a request handler. The body goes out as given, so anything that needs encoding gets encoded by the caller.

## 10.4 Helpers

Three small shims sit alongside the transports so service code stays terse. The client helper wraps connect, disconnect, send and read on any client with timeout discipline and chunked I/O — anyone working at the byte level uses these rather than the raw interface; the SSH server instead queues its encrypted output per session and writes it as the connection takes it, so a slow link holds the client back rather than the service.

Two rules of the TCP layer underneath apply to every transport. A write counts as done once the network stack has queued the bytes: a frame the radio cannot take at that moment is sent again by the stack, so it is not reported as a failure, and a connection that has really gone shows through the stack's error callback. And an established connection is never given up to make room for a new one: listeners sit at the stack's lowest priority and every accepted connection at its highest, so when the connection pool is full (five on an ESP8266) a new arrival is refused, not an open transfer cut. The HTTP helper maps status codes, MIME types and methods to strings. The storage helper maps filenames to MIME types for the file commands and SFTP.

## 10.5 Adding a transport

For CoAP, say:

1. Write the client under `src/transports/coap/`, taking an `iClientInterface*` and exposing begin, send, receive and a callback for responses.
2. Keep it free of the database and the scheduler. A transport parses bytes; it does not own time or persistence.
3. Put the service on top, under `src/service_provider/transport/`. That is where config, scheduling and events live.
4. Add one flag, guard the orchestrator wiring with it, and you are done.

Two rules apply to every transport. The client instance belongs to whoever created it — the transport will not delete it, and a leaked socket per request is fatal on a node that stays up for months. And retries belong to the service, which is the layer that knows whether a failure is worth repeating; transports make one attempt.
