# 18. Troubleshooting & FAQ

Short entries; the explanations live in the sections they point at.

## 18.1 Build and flash

**ESP32: "Sketch too big".**
The default partition scheme's 1.3 MB app slot cannot hold the image of about 1.9 MB. Choose Tools → Partition Scheme → **Minimal SPIFFS (1.9MB APP with OTA)**, which keeps OTA. If your own code then pushes it over, switch off services you do not need ([what costs the most](memory-performance.md#123-the-expensive-features)).

**The build succeeds for a board but the device misbehaves.**
The setup script was never run for that target, so the default-port fallback produced a binary shaped for the wrong board — right code, wrong table set and flags. Run `python3 DeviceSetup.py -d <board>` and rebuild ([§2.5](build.md#25-how-the-default-port-works)).

**The build succeeds but `service list` is empty and no access point appears.**
Same cause seen from the other end: `devices/DeviceSetup.h` still names the previous board. Re-run the script, or delete the file to fall back to the default port.

**`multiple definition of __i_<x>`.**
Either two ports define the same singleton, or a device `.cpp` was included from outside its aggregator chain. Every device translation unit must be reached through exactly one aggregator ([§14.3](porting.md#143-the-two-aggregators)).

**A vendor SDK header is not found while building for a board that does not have it.**
The header leaked above the device layer — the giveaway is that it builds for one port and not another. Push the include down into the port ([§16.10](extending.md#1610-what-to-watch-for)).

**Compile errors inside `pdiutil::function` or `pdiutil::vector` on an unusual target.**
The toolchain is missing the GCC extensions PdiSTL relies on. Use a GCC-based toolchain.

## 18.2 Boot and runtime

**The board restarts about every five minutes.**
It joined a network since it booted and has now been off it for five minutes, so the WiFi service restarts the device to recover (`ALLOW_DEVICE_RESET_ON_WIFI_CONNECT_FAILURES` in `devices/DeviceConfig.h`). Check the router, or set the station again with `net connsta,<ssid>,<password>`. A board that has not joined any network since it booted does not restart; it keeps its access point up and keeps retrying.

**The device factory-resets every five seconds.**
NVM is invalid — a corrupt checksum, or a struct that changed shape since the last flash. With auto-reset on, one cycle recovers it. If it loops, a table no longer fits the space reserved for it ([§5.11](database.md#511-adding-a-table)).

**Boot stops after the banner and no access point appears.**
The station connect is timing out against stale credentials. Hold the flash button for six or seven seconds to factory-reset, join the access point, and set fresh ones.

**`initialize()` completes but no log output appears.**
No console log gate is set, so every call site compiled away. Enable at least the info gate ([§9.3](logger.md#93-flags)).

**The application runs once and stops.**
`PdiStack.serve()` is missing from `loop()`. Without it the scheduler never advances.

**`register_task` always returns -1.**
The slot table is full at 25. Either raise the limit or find the service leaking tasks — `ps` shows the population ([§4.7](scheduler.md#47-behaviour-worth-knowing-before-you-lean-on-it)).

**One service starves the others and the loop feels jittery.**
The scheduler runs one inline task per pass, so a task doing too much per call delays everything behind it. Split it into a state machine or move it to a cooperative lane ([§4.5](scheduler.md#45-what-happens-on-a-tick)).

**Free heap drifts down over days.**
Something is allocating in a hot path. The habits that prevent it are in [§12.4](memory-performance.md#124-heap-discipline).

## 18.3 Network and portal

**Cannot join the `pdiStack` access point.**
The password is `pdiStack@123`, case-sensitive. Confirm the radio is up with `service status WiFi` over serial.

**Cannot reach `http://192.168.0.1`.**
With dynamic subnetting on, the access point may have chosen a different subnet — use the `.1` of whatever your client was given. On an ESP32 that already has its certificate, `http://` forwards to `https://` and the browser asks once to accept the board's self-signed certificate.

**The login form keeps bouncing back to itself.**
The cookie is being rejected or the five-minute idle window expired. Private browsing windows often refuse cookies for a bare IP. After several wrong passwords the login route also holds attempts off for a lockout period — `passwd` over serial or SSH still works meanwhile.

**A form submits and lands back on the login page without saving.**
Its CSRF token no longer matches the session, which is what happens when a page is left open across a logout, a password change or a session expiry. Reload the page and submit it again. A form that does this every time is missing `concat_csrf_input_html_tag()` ([§8.10](web-server.md#810-adding-a-page)).

**A page arrives truncated mid-HTML.**
A `send()` went out without chunking and exceeded the per-send buffer. Compose in header, body and footer ([§8.7](web-server.md#87-composing-a-response)).

**MQTT never connects.**
The client starts only with a client id and a keepalive above 5 seconds; with either missing it does not try at all and `/var/log/syslog.warning` says `MQTT: not connecting, a client id and a keepalive above 5 s are needed`. Set both on the MQTT page — `[mac]` in the client id is replaced with the board's mac. With them set, a broker that refuses the connection is logged in `/var/log/syslog.error`.

**MQTT connects but no callbacks fire.**
Wait for the subscribed callback before publishing on that topic — `Subscribe` reports enqueue, not acknowledgement ([§10.2](transports.md#102-mqtt)).

**OTA polls the server but never updates.**
The device only updates when the server's version is strictly newer than the firmware version compiled into it. Bump the firmware version before publishing ([§6.2.6](services.md#626-otaserviceprovider--__ota_service)).

**Email sends time out.**
SMTP goes out in plaintext, so providers that require TLS on submission will not complete. Use a relay that accepts plain SMTP, or a test sink ([§10.3](transports.md#103-smtp)).

**A mail is not sent.**
`cat /var/log/syslog.error` and look for `Email: not sent, ...`: it names the step the server refused and the code it answered — `the login: 535` is a wrong username or password, `the recipient: 550` an address the server will not deliver to, `no ready answer from <host>:<port>` a server that is not there or not listening. A relay that takes mail without a login needs the username left empty; with a username set the board always logs in.

**HTTPS refuses connections or fails the handshake.**
The certificate and key are missing from the filesystem or sitting at different paths than configured. Upload them to `/etc/http/server.crt` and `/etc/http/server.key`; under mTLS the client CA is needed too ([§8.7.1](web-server.md#871-https-wiring-and-certificates)).

**Outbound HTTPS to an unknown CA fails.**
The bundled client ships with peer verification off so first-boot demos work. For production, point it at `/etc/ssl/ca-bundle.crt` and drop the `setVerifyPeer(false)` line.

**TLS handshakes fail on large records.**
Two ceilings meet here. The handshake runs on a dedicated task stack sized by `TLS_TASK_STACK_SIZE` because the default main stack is too small for an ECDSA sign, and the inbound record buffer defaults well below the 16 KB a peer may emit. Raise the buffer — and pay the heap — only when you cannot control the peer.

**HTTPS works on a roomier board and refuses everything on a tighter one.**
NAPT is almost certainly on as well. The two cannot share that heap ([§12.3](memory-performance.md#123-the-expensive-features)).

**The auto cert-generation listener never fires.**
It hangs off the station getting an IP, so it does not run in AP-only mode. Check with `net ip`.

## 18.4 Shell

**Telnet or SSH rejects the right credentials.**
Every login — serial, Telnet, SSH and the portal — is checked against `/etc/shadow`, so a password changed with `passwd` or from the portal changes it everywhere. If one route still refuses it, set it again over serial with `passwd`.

**SSH accepts no clients.**
The Ed25519 host key is created when the service starts, so this means the write failed — a full or unmounted filesystem. Check `df`, then regenerate:
```
sshkgen t=1,f=b
```
`t=1` is Ed25519 and is instant; `f=b` is the `/etc/ssh` host-key directory.

**`scp -s` is slow.**
Transfers stream in pieces straight to the filesystem, and the speed follows the board: over WiFi an ESP32 moves a file to or from its SD card at about 40 to 52 KB/s, an ESP8266 at about 18 KB/s up and 8 to 35 KB/s down. A link that stalls for a few seconds slows a transfer without ending it; only 30 seconds with nothing moving closes the session. Firmware belongs on OTA ([§7.10](shell.md#710-sftp-and-scp-file-transfer)).

**An upload over an existing file fails with `write remote ...: Failure`.**
The filesystem keeps the old file until the new one is complete, and `scp` writes over the old bytes before it trims the file, so overwriting needs as much free space as the file is long. Remove the old file first, or check `df` has room for it.

**A transfer or an SSH session ended.**
The system log gives the reason for every close the device makes: `cat /var/log/syslog.error` and look for `SSH: session closed, ...`. With nothing logged, the client or the network ended it; on a full disk there is no room for the line either.

**`fedit` will not exit.**
Press Esc for the menu, then `!w` to save and leave or `!c` to discard.

**Tab completion works but the arrow keys don't recall history.**
History is a file, so it needs storage; completion reads the in-RAM registry and works either way ([§7.6](shell.md#76-the-dispatcher)).

**A second telnet client just hangs.**
It won't any more: telnet keeps a pool, and a client arriving when every slot is taken is told `no telnet session available, try again later` and closed. If you see that message, either a session is idle and about to be reaped, or the pool is genuinely smaller than the number of people trying to log in — raise `TELNET_MAX_SESSIONS`, and the session table grows with it ([§7.9](shell.md#79-multi-terminal-session-lifecycle)).

**`echo something > /proc/uptime` prints `cannot write`.**
That is correct — `/proc` is generated and read-only. The message appears when the last block is written, so a redirect that fails halfway still tells you ([§7.7](shell.md#77-pipes-and-redirection)).

## 18.5 Questions that come up

**Can I run two `PdiStack` instances?**
No. One global stack, one singleton per interface and per service — and on these memory budgets there would be no gain.

**Can I use Arduino `String` instead of `pdiutil::string`?**
In your sketch, yes. In framework code, no — its allocator differs per core, which is exactly the portability the framework is built to avoid depending on.

**How long can a command name be?**
Up to 15 characters (`CMD_SIZE_MAX` less one), enough for the linux names. An option key, the `p` in `p=1234`, holds up to 2 (`CMD_OPTION_SIZE_MAX` less one). Both are sized for the tightest board the framework supports, because the cost multiplies across every command slot.

**Why is SSH so heavy?**
Host keys, key exchange, streaming AES and per-session protocol state add roughly 150 KB of flash and 8 to 16 KB of heap per session. That is why SSH wants a board with heap to spare ([§12.3](memory-performance.md#123-the-expensive-features)).

**Can the device be reconfigured without reflashing?**
Yes. Everything persisted in NVM is reachable from the portal, the shell, or a sketch. Flashing only changes defaults ([§3.8](configuration.md#38-defaults-are-not-current-values)).

**Where do logs go?**
Console lines go to serial at 115200. With syslog on, those lines also land in `/var/log/syslog.<level>`, and with forwarding on they go to a collector as well ([§9.6](logger.md#96-where-syslog-goes)).

**How do I turn on TLS?**
Set `ENABLE_TLS_SERVICE`; the port supplies whichever SSL backend it has. Add `ENABLE_HTTPS_SERVER` for the portal, `ENABLE_HTTPS_SERVER_MTLS` for client certificates, and the cert-generation flags for on-boot minting where the port declares support. It is the most expensive flag in the framework ([§12.3](memory-performance.md#123-the-expensive-features)), and on a tight board it cannot share the heap with NAPT.

**How do I provision certificates?**
Where the port can generate them, `tls q=1,t=0,l=256,n=device.local,i=192.168.1.50` writes a self-signed EC pair straight to the configured paths. Otherwise, `python3 scripts/GenTlsCerts.py --dns device.local --ip 192.168.1.50` produces them off-device for upload. Run the script once with `--gen-ca` and reuse that CA for every device, and a client that trusts it trusts your whole fleet.

For a development box none of that is needed, where the port can mint its own: with `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` the mDNS service mints one covering the address and `<hostname>.local` as soon as the station has an IP, and reissues it if either changes.

**Is there a simulator?**
The host port runs the whole framework as a normal process on your computer, so most behaviour can be exercised without a board — the feature suites drive it over a shell, ssh and the portal. What it cannot stand in for is timing, radio and flash wear, so anything that depends on those still needs real hardware.

**How do I unit-test framework code?**
`python3 tests/run_tests.py` — see [§17](testing.md#17-test-suite). The framework compiles against the host port on host x86, so a unit test links the real source with no board attached.

**Where do I report issues?**
GitHub: <https://github.com/Suraj151/pdi-framework>.

## 18.6 When you're properly stuck

```
  1  enable console logging, flash, watch serial at 115200
  2  service list              what actually booted
  3  service status <name>     that service's state and its pids
  4  ps                     %CPU finds the hog; OWN ties tasks to sessions
  5  top i=2000,n=10        the same view over time
  6  service stop / start      freeze and resume, and watch the state column flip
  7  service status DB         NVM validity
  8  reboot                 explicitly, so you keep the serial output
```

If none of that localises it, open an issue with the board and package version, the flags you have set, the serial log through the failure, and the exact sequence that reproduces it.
