# 8. Web Server

The web server is the on-device admin portal. With `ENABLE_HTTP_SERVER` on, the device serves HTTP/1.1 on its access-point address — `192.168.0.1` by default — so a phone or laptop can log in, configure WiFi, drive GPIO, watch a dashboard, upload files and manage MQTT, OTA, email and IoT without a cable. Adding `ENABLE_HTTPS_SERVER` serves the same portal on 443 over TLS and turns `ENABLE_TLS_SERVICE` on for it; port 80 stays open only to redirect `http://` callers up to `https://`, so a typed-in plain address still lands on the secure portal. No controller, page or route changes, only a different `begin()` call and a certificate on disk.

It is the largest subsystem in the framework, but it is built from a handful of pieces: an orchestrator, a route registry, a middleware check, a resource provider that owns the response buffer, one controller per feature, and HTML fragments in flash.

Start reading at [src/webserver/](../src/webserver/).

## 8.1 Layered model

```
   iHttpServerInterface           bytes in and out, route registration
        │  on(uri, cb) · arg() · header() · send()
        ▼
   HttpServer                     owns one controller per feature, each flag-gated
        │  boots every controller once
        ▼
   Controllers                    home · login · dashboard · wifi · ota · gpio
        │                         mqtt · email · iot · storage
        ├─ register_route(uri, fn, middleware, redirect) ──▶ RouteHandler
        │                                                       │
        │                                                   Middleware
        │                                                    ├─ none
        │                                                    ├─ auth ─▶ session lookup, else 301
        │                                                    └─ api  ─▶ session lookup, else 401
        │                                                       └─ POST ─▶ CSRF token check
        │
        └─ build the response ──▶ WebResourceProvider
                                    ├─ the page buffer and the server pointer
                                    ├─ shortcuts into the database service
                                    └─ HTML fragments in flash: header · page · footer
```

A request enters through the port, which matches the URI to a registered lambda. The lambda runs its middleware; on a pass the controller method runs, pulls config from the database service, composes HTML from the fragments and sends it. On a fail the middleware sends a redirect to `/login` and the controller never runs.

A request whose handler sends nothing, because its page buffer could not be had at that moment, is answered `503 Service Unavailable` and the connection is closed, so the browser can retry at once rather than wait on an open connection.

## 8.2 The orchestrator

`HttpServer` is a service provider that owns its controllers as value members rather than as singletons. Home, dashboard and login are always present; the rest appear with their feature flag — OTA, WiFi, GPIO, MQTT, email, IoT and storage.

`initService` stashes the server pointer, hands it to the resource provider, and walks the controller registry calling `boot()` on each. Every controller put itself into that registry when it was constructed, so registration needs no wiring.

The last thing `initService` does is bind the listener, and that is the only place TLS shows up:

```cpp
#if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
this->m_server->setServerCertificatePath(TLS_DEFAULT_SERVER_CERT_PATH);   // /etc/http/server.crt
this->m_server->setServerPrivateKeyPath(TLS_DEFAULT_SERVER_KEY_PATH);     // /etc/http/server.key
  #ifdef ENABLE_HTTPS_SERVER_MTLS
this->m_server->setClientCertificateAuthorityPath(TLS_DEFAULT_CLIENT_CA_PATH);
  #endif
this->m_server->begin(HTTPS_DEFAULT_PORT, /*secure=*/true);               // 443
#else
this->m_server->begin(HTTP_DEFAULT_PORT);                                 // 80
#endif
```

When it binds 443, the secure `begin()` also stands up a bare TCP listener on 80 whose only job is to answer each request with a redirect to the `https://` URL, so the plain port is never left dark for a browser that omits the scheme. Nothing else in the subsystem is conditional on TLS. The per-tick `handle_clients()` is a single call into the port; all the work happens inside the route lambdas.

## 8.3 Routes

URIs are constants in one header, so a typo is a compile error rather than a dead link.

| Route | Purpose | Gate |
|---|---|---|
| `/` | menu landing | — |
| `/login` | form and its POST | — |
| `/logout` | drop the session | — |
| `/login-config` | change your own password | auth |
| `/dashboard` | live device summary | auth |
| `/listen-dashboard` | one aggregated dashboard payload | api |
| `/wifi-config` | station and AP form | auth |
| `/ota-config` | update server url, version, and flashing an image from storage | auth |
| `/email-config` | SMTP credentials | auth |
| `/gpio-manage`, `/gpio-server`, `/gpio-config`, `/gpio-write`, `/gpio-event` | GPIO panel and its subforms | auth |
| `/gpio-monitor` | live analog graphs | auth |
| `/listen-monitor` | graph and pin updates | api |
| `/mqtt-manage`, `/mqtt-general-config`, `/mqtt-lwt-config`, `/mqtt-pubsub-config` | MQTT forms | auth |
| `/device-register-config` | IoT registration | auth |
| `/storage`, `/storage-fileupload`, `/storage-filelist`, `/storage-filedel` | file browser, upload and delete; a listed file downloads at its own path, refused with `401` when the logged-in user may not read it. Every directory and mount point opens: an empty one says so, a list that cannot be read says so with its status, names show whole and sizes read up to E | auth |

The file browser lists each entry the way `ls -l` does — a permission string such as `drwxr-xr-x`, then owner and group resolved to names through the user store, then size. Every operation runs as the signed-in user, so a listing, a delete or an upload succeeds exactly where the same account would succeed from the shell, and an upload lands owned by whoever uploaded it. A refusal is reported as a denied permission rather than a generic failure.

The dashboard gathers what the other subsystems already know into one card grid, refreshed from a single `/listen-dashboard` payload every three seconds. Link state sits in the page heading as symbols: wifi arcs lit by signal strength with the RSSI beside them, and a globe that dims when the internet probe last failed. Storage and heap are rings — the storage ring is the used share of the root filesystem, the memory ring is how much of the free heap is one contiguous block, which is the number that matters when an allocation fails on a device still reporting plenty free. Below them sit uptime with the four busiest tasks in the columns of `ps` — pid, name, state letter, lifetime CPU share — then every authenticated session with the transport it arrived on, so a serial login, an SSH session and a portal login appear side by side with their login and idle times. Configured pins render as tiles carrying mode and current state (`DOUT · HIGH`, `DIN · LOW`, `AOUT · 180`), and an analog pin adds a rolling trace against a labelled axis, held on the client so the device only ever sends the present reading. Each card is fed by its own feature flag, so a build without storage, GPIO or the auth service simply serves fewer of them.

Rows that repeat are arrays rather than objects, which keeps a full payload — radio, filesystem, heap, tasks, sessions, pins and attached stations — around 400 bytes, and the response string is reserved up front so it never grows in steps while being built.

Registering one takes a URI, a callback, an optional middleware level and an optional redirect target. There is also a hook for the not-found handler, which the home controller owns.

## 8.4 Controllers

A controller inherits the base — which registers it — holds pointers to the resource provider and the route registry, and implements `boot()`:

```cpp
class LoginController : public Controller {
public:
    LoginController() : Controller("LoginController") {}

    void boot() override {
        m_route_handler->register_route(WEB_SERVER_LOGIN_ROUTE,
                                        [&] { this->handleLoginRoute(); });
        m_route_handler->register_route(WEB_SERVER_LOGOUT_ROUTE,
                                        [&] { this->handleLogoutRoute(); });
        m_route_handler->register_route(WEB_SERVER_LOGIN_CONFIG_ROUTE,
                                        [&] { this->handleLoginConfigRoute(); },
                                        AUTH_MIDDLEWARE);
    }
};
```

Ten controllers ship, one per feature area, each owning the routes in its own prefix. The GPIO and MQTT ones are by far the largest, because they carry the most form state.

Since every route ends up in one global registry, URI constants have to be unique — two controllers claiming the same path means the second one wins.

## 8.5 Middleware

Four levels. A route registered without one is public. The auth level asks the session handler whether the request carries a live session and, if not, redirects to the configured target so the route never runs. The API level makes the same check but answers `401`, which is what the polling endpoints want — they are consumed by script, not navigated to.

The root level is the auth level plus one question: is this session root's? If not, the middleware answers with a single shared "root privilege required" page carrying a link home, and the handler never runs. That is what keeps the portal and the filesystem agreeing — a settings file is `0600 root:root`, so the page that edits it is root's too.

```cpp
register_route(WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE,
               [&]() { this->handleMqttGeneralConfigRoute(); },
               ROOT_AUTH_MIDDLEWARE);
```

Because the refusal is a *standard* page rather than the controller's own, the middleware can render it, which is why restricting a page costs one enum value and no handler code. Gate the pages that change something, never the menu that reaches them: the GPIO menu stays on the auth level so a non-root user can still get to the pin monitor, and each configuration card behind it refuses on its own. Watching is not configuring.

All guarded levels then apply one more rule: a POST must carry the CSRF token of its own session, or it is refused — `403` on an API route, a redirect on a page. Keeping that check in the middleware rather than in each handler means a newly added form is covered by default; it only has to render the field.

Forms get the field from a single shared helper, `concat_csrf_input_html_tag()`, which sits with the other markup builders and writes its markup from flash-resident constants. Pages that submit through script read the same value with `get_csrf_token()`.

Middleware lives on the session handler rather than as a separate chain, so a new level means extending the enum and adding a branch — routes then opt in by naming it.

## 8.6 Sessions

Sessions live on the server. `WebSessionManager` owns a small fixed table of slots — two by default, `WEB_MAX_SESSIONS` — and the browser only ever holds an opaque token.

Logging in verifies the credentials through the auth service, takes a slot, and draws two independent random values from the hardware RNG: the session token that goes into the cookie, and a CSRF token handed to every form that session renders.

```
Set-Cookie: pdi_session=<32 hex>; Max-Age=300; Path=/; HttpOnly; SameSite=Strict[; Secure]
```

The cookie name and max age come from the credential table — `pdi_session` and five minutes. `Secure` is appended when the request itself arrived over TLS, rather than when the build has TLS compiled in, because a browser silently drops a `Secure` cookie delivered over plain HTTP. Each guarded request parses the `Cookie` header by exact name, matches the token with a constant-time compare, and enforces both an idle timeout (the cookie max age) and an absolute cap (`WEB_SESSION_ABSOLUTE_MAX_AGE`, eight hours). Logging out, or changing a password, releases the slot, so a captured cookie stops working at that moment rather than at expiry.

Both ends of that window have to slide together. The server refreshes its idle stamp on every request, but the cookie carries a fixed expiry from the moment it was written, so a session that is still in use is handed a fresh cookie once it passes half its life — otherwise the browser drops it mid-session and the user is bounced to the login form while the session is still live. The reissue is rate-limited to the half-life rather than sent on every response, so a page polling every few seconds does not pay for the header each time.

A slot is reclaimed when its session expires, and any request sweeps the whole table rather than only the slot whose token matched. Without that, a client that simply walks away leaves its slot held until the next login happens to notice — which on the default two-slot table costs half the capacity.

A web session *is* a `session_t` — the same structure the terminals use, extended with the two tokens. That means a validated request can be published through `SessionManager` for the life of the handler, which is what makes the VFS resolve file operations as the logged-in web user: uid, gid and umask are the session's, not root's. The route wrapper detaches it again once the handler returns, so a web session is never current while terminal work runs.

Failed logins are counted; past `WEB_LOGIN_MAX_ATTEMPTS` the login route refuses attempts for `WEB_LOGIN_LOCKOUT_DURATION`. The counter is device-wide rather than per-client, because the server interface exposes no client address.

## 8.7 Composing a response

The resource provider holds the server pointer for the current request, the page buffer, and shortcuts into the database service so controllers never include database headers.

Pages are raw HTML strings kept in program memory and sent as header, body and footer:

```cpp
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml, /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, bodyHtml,   /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml, /*chunked=*/true);
```

The header and footer are shared by every page; the middle is whichever page is being served. Sending in chunks is what lets a page exceed the 1800-byte single-send buffer, which is why every composed page uses it.

Three helpers carry the dynamic markup: a page builder that turns C structs into form inputs, tables and option lists; a set of tag and attribute constants that keep allocations down; and an inline SVG icon set.

The shared header carries the styling for every page, so the look is defined once. Colours are CSS custom properties with a light and a dark set: the dark set applies through `prefers-color-scheme` until the visitor picks a side with the toggle in the top corner, after which a `data-theme` attribute on the root element wins and the choice is remembered in `localStorage`. Success, error and warning share that treatment — a flash message names a status class and takes its text colour from a token, over a translucent tint that composites against whichever card colour is in force, so one rule reads correctly in both schemes. Layout is sized for a phone first and widens with the viewport, with controls given larger hit targets on coarse pointers; pages that need the room, such as the file browser, open a wider container. Because the header is larger than one chunk, it is emitted as several flash-resident fragments, each under the single-send buffer.

### 8.7.1 HTTPS wiring and certificates

The same server implementation runs in TLS mode, with responsibility split like this:

```
  HttpServer::initService     sets cert, key, optional client CA, then begin(443, secure)
        │
  iHttpServerInterface        adds the TLS setters and the secure flag; they are no-ops
        │                     in a non-TLS build, so existing ports compile untouched
        ▼
  HttpServerInterfaceImpl     listener switches to the TLS server instance; each accepted
        │                     client still looks like an ordinary client to the parser
        ▼
  port TLS backend            whichever the port supplies — loads PEM from the
                              filesystem at the configured paths
```

| Path | Default | Purpose |
|---|---|---|
| server certificate | `/etc/http/server.crt` | PEM, may be a chain |
| server key | `/etc/http/server.key` | PEM, EC or RSA, `0600` |
| client CA | `/etc/http/client-ca.crt` | only under mTLS |

Upload those over SFTP after first boot, `chmod 0600 /etc/http/server.key` so only root can read the key, and reboot; the listener picks them up on the next start. The directory is created for you.

Where the port supports on-device generation, the certificates come from the `tls` command, or are minted automatically by the mDNS service once the station has an address, covering both the IP and `<hostname>.local`. Where it does not, they come from `scripts/GenTlsCerts.py` off-device and the upload above is the only way they arrive — there is no `tls` command to fall back on.

**Serving HTTPS costs a live TLS session on top of whatever else is running.** On a device with heap to spare that is a flag change; on one already near its limit, expect to compile other services out to make room, and read [§12.3](memory-performance.md#123-the-expensive-features) before assuming it fits.

One header is worth a decision rather than a default: `Strict-Transport-Security` is sent only when its max-age is non-zero, and it ships as zero. Turn it on once you have a CA-signed certificate. With a self-signed one, the browser will pin HTTPS and refuse the click-through until the pin expires.

## 8.8 Request lifecycle

```
  client request
      │
  handleClient()                parse method, URI, headers, args
      │
  route lambda                  the one Controller::boot() registered for this URI
      │
  middleware                    auth? no live session → 301 to /login, stop here
      │                         POST? no matching CSRF token → refused, stop here
      │                         on a pass the session becomes current
      │
  controller method             read args, load config, on POST validate and persist,
      │                         then send header + page + footer
      │
  release                       the session stops being current
      ▼
  send(code, mime, body, chunked)  ──▶  bytes back to the client
```

Routes answer `GET`, `POST` and `HEAD`. Any other method is refused with `405` and an `Allow: GET, POST, HEAD` header before it reaches a route, so a `PUT` or `DELETE` can never run a page's handler as if it were a `GET`.

A request line longer than `HTTP_REQUEST_LINE_MAX` (1024 bytes, `HttpConfig.h`) is read to its end without being kept, logged, answered `414 URI Too Long` and the connection closed. Everything a request held — its path, headers, query, form fields and body — is released once it is answered, so one large request never stays in memory after it.

## 8.9 Three routes worth tracing

**`/wifi-config` POST.** Auth passes, the controller reads the station and AP arguments, loads the current WiFi table so untouched fields survive, applies the new values, saves, and renders the success page. A value that does not fit saves nothing, and the page answers with the limits instead: `Not saved: a name takes 4 to 29 characters, a password up to 29 and an address up to 19.` (the numbers come from `WIFI_CONFIGS_BUF_SIZE`, `MIN_ACCEPTED_ARG_SIZE` and `WIFI_CONFIGS_IP_BUF_SIZE`). The device registration form answers a value too long for it the same way, naming the field and its limit, such as `Device id is too long: up to 99 characters.` The actual reconnect is scheduled a tick later — the response has to flush before the radio drops out from under it.

**`/listen-monitor`.** The browser polls; each request reads the analog pin and returns a small JSON object with the pin and its value. It carries the API gate, so an expired session gets a `401` and the page sends the browser back to the login form. That is the whole live-graph mechanism.

**`/storage-fileupload`.** The body is streamed to a staging file as it arrives, so upload size is bounded by storage rather than by RAM. Incoming bytes are gathered into blocks of `HTTP_UPLOAD_WRITE_BLOCK_SIZE` (2 KB) before each write, or a smaller block down to `HTTP_UPLOAD_WRITE_BLOCK_MIN` when the heap is short. The file ends only at the closing delimiter, however the network splits the bytes across reads, and every line around it — the request headers, each part's headers and the blank line that ends them, a field's value, a boundary — is read whole, waiting for the rest of a line that arrives in two pieces, so a cut line is never taken for the end of the headers or for file data. A request header line keeps its first `HTTP_HEADER_LINE_MAX` (512) bytes. A read whose buffer the heap cannot grow at that moment leaves the rest of the bytes waiting on the connection for the next pass, so a busy heap slows an upload rather than losing part of it. When storage refuses a block, or the transfer stalls, the upload is abandoned and its staged file deleted, so a file is placed whole or not at all. The form's `dir` field names the directory the file is bound for, and the file is staged on that directory's filesystem: in `.upload` at the top of a disk mounted there, so an upload to a card is bounded by the card and moved into place without a copy, and under `/temp` for anything else. Only the last part of the name the file came with is kept, so a name carrying `../` cannot place it outside staging. The staging directory is created on demand if it is missing. The session cookie is checked before anything is staged, and a request that carries no live session stores nothing. Parsing finishes before the handler runs, so the staged file lands owned by root. Once the CSRF check passes, the controller moves it to the requested directory — picking a free name rather than overwriting — and assigns it to the logged-in user, which is what lets that user manage the file afterwards. The move is the server's own step, taken only for a file directly inside the staging directory and only when the user may write the directory it lands in; a name in the form that points anywhere else moves nothing. A move or delete that the user's permissions do not allow is reported back on the page rather than silently dropped. A staged file that is not moved is deleted, and a disk's `.upload` directory is removed once nothing else is staged in it.

## 8.10 Adding a page

For a `/metrics` page:

1. Add the route constant next to the others.
2. Add `MetricsPage.h` with the body HTML as a program-memory constant.
3. Write the controller:
   ```cpp
   class MetricsController : public Controller {
   public:
       MetricsController() : Controller("MetricsController") {}
       void boot() override {
           m_route_handler->register_route(WEB_SERVER_METRICS_ROUTE,
                                           [&]{ this->handleMetrics(); },
                                           AUTH_MIDDLEWARE);
       }
   private:
       void handleMetrics() {
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml, true);
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, metricsHtml, true);
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml, true);
       }
   };
   ```
4. Add it as a value member of `HttpServer`, behind a flag if it is optional, and include its header. Nothing else — the base constructor registers it.
5. Add a menu entry so people can reach it.

If the page carries a form that posts back, call `concat_csrf_input_html_tag()` while composing it. The middleware refuses an authenticated POST without a valid token, so a form that omits the field will submit and be turned away.

Two habits keep the portal responsive. Pass page strings straight to `send()` rather than copying them into RAM; it reads flash directly. And keep controller methods short — `handle_clients()` runs on every pass of the main loop, so a route that needs real work should schedule it and return immediately rather than blocking the loop while it runs.
