# Portable Device Interface Stack

PDIStack Framework covers all essential things to build IoT applications easily.
Basically it is designed on the top of portable interface layers that any device can create their interface from. esp*-arduino examples provided to make things easy to understand for developers.

PDI Framework is a modular C++ stack for embedded and IoT systems, supporting services like WiFi, HTTP/S client & server, MQTT client, SSH server, Telnet server, OTA, GPIO, Serial, Terminal over serial/telnet/ssh, SFTP, SMTP client and much more handled with configurable Task Scheduler.

**Features:** Secure communication, device management, command-line/web interfaces, Configurable Task Scheduler (inline/cooperative/preemptive), and cross-platform support.

**PDI Framework Structure**


<p align="center">
  <img width="500" src="https://github.com/Suraj151/pdi-framework/blob/master/doc/pdi-framework.jpg">
</p>


Arduino has provided user-friendly libraries that use different devices SDK APIs at bottom. Since arduino has made its easy iot development environment impact over developers, it's easy for them to develop applications with Arduino ide.

PDI framework sits on the top of service layers which internally uses the portable interface. So the device which needs to run this framework should write/implement the interface provided.

For a deeper architectural breakdown — layered model, the device/interface/service/utility split, runtime lifecycle, and global symbol naming — see [§1. Architecture Overview](#1-architecture-overview).

# Installation

***installation from Arduino library manager for this library is not supported due to autogen scripts which required to generate the device specific config and DB table sources. Also the external libraries this framework depends on which is part of git submodules config in this repository.*** The reasons and the planned fixes are detailed in [§14.7 Why the Arduino Library Manager isn't supported](#147-why-the-arduino-library-manager-isnt-supported).


**install manually**

To install manually, clone this repo to devices libraries path. for example with Esp8266 device it will be copied in esp8266 libraries path 
( in windows 10 generally path is like ==> C:\Users\suraj\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\x.x.x\libraries...).

( in Ubuntu generally path is like ==> ~/Arduino/libraries).

Follow device official docs to install device specific toolchains. for example to install esp32 device specific toolchains you can follow https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#linux link. similarely for other devices if provided.

**install external dependency**

Currently for some devices, this library is using external dependencies which are added in git submodule config. To install this submodule, open a terminal in this framework library root directory path and execute command ``git submodule update --init --recursive`` 


**AutoGen Script**

Before start compiling for the specific device make sure that device specific auto gen source files has been generated and device has been selected in device config file (mentioned in below note). Currently database table source files are auto generated.

Open terminal and navigate to `scripts` directory in this library and then run below device setup specific script.

`` python3 DeviceSetup.py -d {DEVICENAME}
``

replace DEVICENAME in above command with specific device folder name for which build has to be done. 

For example if we want to build for arduino uno then command will be

`` python3 DeviceSetup.py -d arduinouno
``

**Note :** Above script will create DeviceSetup.h file in devices folder contains the device macro in uppercase format prepended with DEVICE_ word. for example if we are using arduinouno as per mentioned in above command then it will generate DEVICE_ARDUINOUNO macro which has been used in framework to enable or disable device specific features.

The full build flow — what every script does, what's git-ignored, submodule handling, board-package versions, and how to add a new device port — is covered in [§14. Build & Toolchain](#14-build--toolchain) and [§2. Device Layer & Porting Guide](#2-device-layer--porting-guide).

Currently below devices has been supported

| device | Installed Board version in Arduino IDE |
|--------|---------------|
| arduinouno | 1.8.6 |
| esp8266 | 3.1.2 |
| esp32 | 3.3.3 |

# Usage

Restart the arduino ide and navigate to File->Examples->pdi-framework->PdiStack example compile and upload. The bundled examples are walked through line by line in [§15. Examples Walkthrough](#15-examples-walkthrough).


* after flash and initializing device completely, check in pc/mobile wifi list if **pdiStack** name appear.
* select it and enter default password **pdiStack@123**.
* finally after succesful connectinon to device open browser, type **192.168.0.1** in address bar and press enter
* you will directed to login screen, enter default username and password **( username: pdiStack, password: pdiStack@123 )**
* now you will able to see below menu options

<p align="center">
  <img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/portal_home_menu.png">
</p>


* Below are few more examples as you navigate inside


<p align="center">
  <img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/gpio-control-menu.png">
</p>


<p align="center">
  <img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/mqtt-submenu.png">
</p>


<p align="center">
  <img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/storage-home.png">
</p>

you can play with all settings. you can modify configs by making changes in files of src/config folder. Go to wifi settings and change the default station ssid, password to connect to your station. you can also change ssid and password for access point. device will reset once after you submit wifi settings, i.e. you have to reconnect device.

**Note** that by default session will active for 300 seconds once login for web portal, you can change its timeout in server config file. The full configuration system — every `ENABLE_*` flag, the per-service config struct contract, and dependency rules — is covered in [§4. Configuration System](#4-configuration-system).

**Note** not all devices having wifi/network feature to run web server on device as mentioned in above esp8266 example. e.g. arduino uno device wont have the web server enabled due to lack of network feature.

# Terminal

Once flashed open serial port on putty with default baud rate of 115200. It will prompt for login where you can enter default(in case not modified) user & password ( user: **pdiStack**, pass: **pdiStack@123** ). Once login success you can use below available commands in terminal.

**Terminal can be accessed over telnet/ssh running on port 23/22 respectively**. It will provide way to access device remotely. Just open telnet/ssh session on putty/terminal with device ip, you will be able to access terminal.

How the same CLI is multiplexed across serial / telnet / SSH, the input-sequence handling (TAB autocomplete, history, CTRL+C), and how to add a new command are detailed in [§10. Command Line / Terminal](#10-command-line--terminal).

#### SSH file transfer
Framework also support file transfer over SSH tunnel. Both single-file `scp` and an interactive `sftp` client session are supported, all carried by the same SFTP subsystem under the hood.

For example if we want to upload the test.txt file to esp* device sorage (having pdiStack user) from our desktop over ssh. then use below command.

```
scp -s <desktop-path-to-test.txt> pdiStack@<device-ip>:<dest-path-to-store-test.txt>
```

Similarely, if we want to download the test.txt file from esp* device sorage (having pdiStack user) to our desktop over ssh. then use below command.

```
scp -s pdiStack@<device-ip>:<dest-path-to-test.txt> <desktop-path-to-store-test.txt>
```

You can also open an interactive SFTP shell to browse and manage the device's storage:

```
sftp -P 22 pdiStack@<device-ip>
```

The following SFTP operations are implemented (sufficient for OpenSSH `sftp` client interactive use):

| Operation | sftp command | Purpose |
|---|---|---|
| `REALPATH` | `pwd`, `cd` | resolve relative / `.` / `..` paths to absolute |
| `STAT` / `LSTAT` / `FSTAT` | `cd`, `ls`, `stat` | file/dir attributes (size, type) |
| `OPENDIR` / `READDIR` / `CLOSE` | `ls`, `ls -l` | directory listing (paginated, 16 entries/response) |
| `OPEN` / `READ` / `WRITE` / `CLOSE` | `get`, `put` | file read / create / write |
| `MKDIR` / `RMDIR` | `mkdir`, `rmdir` | directory create / remove |
| `REMOVE` | `rm` | delete file |
| `RENAME` | `rename` | rename file or directory (does not overwrite) |
| `SETSTAT` / `FSETSTAT` | `chmod`, `chown` | accepted but no-op (FS has no perm/time storage) |
| `READLINK` / `SYMLINK` | — | reported as unsupported (no symlink layer) |

Idle SFTP sessions are reaped automatically after the configured timeout (default 60 s) so a suspended or dead client never wedges the single-session slot. Interactive SSH shell sessions are NOT idle-reaped — only the SFTP subsystem is.

**Note** that framework handles the ssh chunks in smaller size which can make the file transfer little bit slower. on top if we are overwriting existing file then it can adds the file edit overhead as well. So speed can be as slow as 0.2kbps and as fast as 1kbps. The reasoning and trade-offs of the SSH/SFTP design are covered in [§7.2.14 SSHServer](#7-service-providers).


<p align="center">
  <img width="500" src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal.png">
</p>


<p align="center">
  <img width="500" src="https://github.com/Suraj151/pdi-framework/blob/master/doc/ssh-telnet-terminal.png">
</p>


| Command                                  | Options                             | Brief                                                                                                                                                                                                                                                                                                                                                                                                                       |
|------------------------------------------|-------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ls                                       |                                     | List the file's or dir's in current directory. e.g. **ls**                                                                                                                                                                                                                                                                                                                                                                               |
| mkd \<directory_name>                     |                                     | Make directory with name provided in terminal. space are not allowed in name. e.g. **mkd /home/scripts**                                                                                                                                                                                                                                                                                                                                                |
| mkf \<file_name>                          |                                     | Make file with name provided in terminal. space are not allowed in name. e.g. **mkf /home/notes.txt**                                                                                                                                                                                                                                                                                                                                                     |
| mv \<old_path_name> \<new_path_name>     |                                     | Move/Rename the file/dir. space not allowed in name. e.g. **mv /home/a.txt /home/b.txt**                                                                                                                                                                                                                                                                                                                                                                         |
| cp \<old_path_name> \<new_path_name>     |                                     | Copy the file. e.g. **cp /home/a.txt /home/b.txt**                                                                                                                                                                                                                                                                                                                                                                          |
| pwd                                      |                                     | It will print present working directory path. e.g. **pwd**                                                                                                                                                                                                                                                                                                                                                                                |
| rm \<file_or_dir_name>                    |                                     | Remove the file or directory provided. e.g. **rm /home/notes.txt**                                                                                                                                                                                                                                                                                                                                                                                       |
| cat \<filename>                             |                                     | Read and print the file content of given filename over terminal. (Renamed from `fread`.) e.g. **cat /home/notes.txt**                                                                                                                                                                                                                                                                                                                                    |
| fwrite \<filename>                           | f=\<filename> v=\<value>             | Open file for write. Note that this command will not show file existing content, rather it will open file to append the content we write. once writing done we can use ESC key to write and quite or cancel the writing. The filename can also be supplied positionally; once the file is open, content is read line-by-line until ESC. e.g. **fwrite /home/notes.txt** or **fwrite f=/home/notes.txt v=hello**                                                                                                                                                                                                                       |
| head \<filename> [N]                        |                                     | Print the first N lines of the file (default 10). Walks the file forward in a single pass — fast even on large files. e.g. **head /home/log.txt 5**                                                                                                                                                                                                                                                                                                       |
| tail \<filename> [N]                        |                                     | Print the last N lines of the file (default 10). One offset lookup then forward read; constant memory regardless of file size. e.g. **tail /home/log.txt 5**                                                                                                                                                                                                                                                                                              |
| wc \<filename>                              |                                     | Print line, word and byte counts of the file in `LINES WORDS BYTES filename` order, matching Linux `wc`. e.g. **wc /home/log.txt**                                                                                                                                                                                                                                                                                                                    |
| df                                          |                                     | Print filesystem total / used (with %) / free in bytes. e.g. **df**                                                                                                                                                                                                                                                                                                                                                                     |
| hexdump \<filename>                         |                                     | Hex view of a file: offset, 16 bytes hex, ASCII column — `hexdump -C` layout. Streams via 16-byte readFile chunks. e.g. **hexdump /home/bin.dat**                                                                                                                                                                                                                                                                                                          |
| grep \<pattern> \<file_or_dir>              |                                     | Search for pattern in file(s). If a directory is given, walks recursively. Output `path:line:col:content` matches the `file:line:col:` jump format used by vim/vscode/emacs. Supports a small regex subset: `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` `\\<char>`. No alternation, grouping, or backreferences. e.g. **grep ^ERROR /home/log.txt**                                                                                                                                |
| cls                                      |                                     | Clear screen command. e.g. **cls**                                                                                                                                                                                                                                                                                                                                                                                                        |
| cd \<directory_path>                      |                                     | Change Directory to the provided directory path. e.g. **cd /home/scripts**                                                                                                                                                                                                                                                                                                                                                                             |
| gpio p=\<pin>,m=\<mode>,v=\<value>          | p=\<pin> m=\<mode> v=\<value>          | To perform gpio operations we can use this command. p\<pin> option can be the GPIO pin number to operate with. m\<mode> is the GPIO mode number we want to use. available GPIO modes are OFF=0 DIGITAL_WRITE=1 DIGITAL_READ=2 DIGITAL_BLINK=3 ANALOG_WRITE=4 ANALOG_READ=5 . v\<value> is the value respective to the mode used.  e.g. If we want to blink GPIO 4 with 500ms the we can use command like:  gpio p=4,m=3,v=500  |
| srvc s=\<service_number>,q=\<query_number> | s=\<service_number> q=\<query_number> | Services related command. currently supporting to get running service config and status.  service number can be get from srvc l command which will list the running services with their id numbers below are the query number we can use in command QUERY_CONFIG=1 QUERY_STATUS=2. e.g. **srvc s=1,q=2**                                                                                                                                           |
| scht                                     | l\<list>                             | List the running Scheduler tasks. e.g. **scht l**                                                                                                                                                                                                                                                                                                                                                                                            |
| ssh q=\<query_number>,t=\<algo_type> | q=\<query_number>  t=\<algo_type>  | SSH command according to provided query and options. e.g. if Query number is 1 (SSH_COMMAND_QUERY_KEYGEN) then it wwill create the key pairs of provided algo_type option. e.g. **ssh q=1,t=2**                                                                                                                                                                                                                                                                                                                                                                                           |
| net \<options> | ip, scansta, connsta | Network command to query the network parameters. we can provide options to query parameters. e.g if we provide **ip** option we may get the info of station to which device is connected and its ip as well as access point name and ip. If we provide **scansta** option it will list out the available station networks. Then we have **connsta** command which can be used to connect with station provided station name and its password e.g. **net connsta,\<stationname>,\<stationpassword>**                                                                                                                                                                                |
| reboot |  | Reboot the Device. e.g. **reboot**                                                                                                                                                                                 |
| watch | c=\<command_to_execute_with_its_options_periodically> i=\<interval_in_milliseconds>  n=<number_of_iterations> | Watch command will bring functionality where user need to execute provided command with their options peridically. optionally we can provide the interval and iterations for within which command shall run periodically. default interval is 1 second and infinite iterations. we can stop running watch command with ctrl + c key combination. options should be seperated with semicolon. for example we want to print the network ip every 3 second upto 10 iterations then we can use watch command as: watch c=net ip; i=3000; n=10 |
| iot \<options> | setid, getid, sethost, gethost | Device iot command to setup iot config. To set/get Device Unique ID we can use **setid/getid** command respectively. To set/get HTTP Host of IOT we can use **sethost/gethost** commands respectively. Let say if we want to set id then we have to use command as : **iot setid,\<DeviceuniqueID>**, similar way when we want to set host we can use command as : **iot sethost,\<IotHTTPHostAddress>**                                                                                                                                                                                |
| help                                        |                                     | List every registered command. Available before login so the user can discover commands (including `login`). Tab-completion also works on partial names. e.g. **help**                                                                                                                                                                                                                                                                    |
| uptime                                      |                                     | Print time since boot in the format `up Xd Yh Zm Ws` (days, hours, minutes, seconds). Backed by `__i_dvc_ctrl.millis_now()`; wraps at ~49.7 days. e.g. **uptime**                                                                                                                                                                                                                                                                            |
| tls q=\<query>,t=\<algo>,l=\<bits>,n=\<CN/DNS>,i=\<IPv4> | q (1=CERTGEN), t (0=EC, 1=RSA), l (key bits / curve size), n (subject CN or DNS SAN), i (IPv4 SAN) | TLS cert generation on-device. Only available on esp32 builds with `ENABLE_TLS_CERT_GENERATION`. Output goes to `TLS_DEFAULT_SERVER_CERT_PATH` / `TLS_DEFAULT_SERVER_KEY_PATH` (`/etc/http/server.{crt,key}` by default). e.g. **tls q=1,t=0,l=256,n=device.local,i=192.168.1.50**                                                                                                                                                       |

# Services

PDI framework provides some basic services that required to develop simple iot application. All services are available globally to each other. A short description of each service follows; the per-service reference (dependencies, init flow, public API, CLI/web surface) is in [§7. Service Providers](#7-service-providers). The services are

* **MQTT Service:**
This lightweight messaging protocol can be used to monitor or operate device itself or sensors that are connected to the device.
To configure and test MQTT go on device local server and select MQTT section under main menu.


* **OTA Service:**
Over The Air (OTA) feature has ability to update the device firmware remotely. By default OTA configurations are accessible with local server. OTA service is uses firmware version to decide whether start to update or not. OTA server can be set in OTA configuration which is accesible through local server.

    we need to set below route at server

    GET route format as ==> http://server.com/ota?mac_id=xx:xx:xx:xx:xx:xx&version=2019041100 --// this link is called by device on every x seconds provided in OTA configuration with its mac_id and current version ( available in global configuration ) as parameters. Response should be in json as ==> { latest : 2019041101 } which returns latest firmware version available on server

    by default update start only if device current firmware version is older than received firmware version from server.

    when device start the update process after knowing its current firmware version is older it looks for the downloadable file from the same server in format given below

    server address / bin / device mac address / latest firmware version .bin file
    e.g. http://server.com/bin/xx:xx:xx:xx:xx:xx/2019041101.bin


* **ESPNOW Service:**
This service is extended version of ESPNOW feature available in esp with some easy to use api. with help of this feature we can build mesh networking, broadcasting etc n/w as per requirements. this service is not configurable from server for now. but you can manage it with easily available api of this service.

* **WiFi Service:**
This service provides simplified api's to dynamically interact with wifi devices on practical field. it has internet based connection ability over same network configs devices which are usefull in mesh scenarios. it also has ability to enable dynamic subnetting heiraechy where each individual device sits in different network and knows how far (in hop distance manner) he is from main hub centre.

* **GPIO Service:**
GPIOs are actually going to interact with sensors. We can read sensor or we can drive appliances with the help of this GPIO services. More on this GPIO service can work with MQTT/HTTP transports provided to monitor/control the GPIO's remotely

* **MAIL Service:**
MAIL service is uses SMTP transport to connect and send mail to any account. you should have a SMTP server account credentials that device uses to send mail. to set configuration goto local server and select Email section under main menu.
you can test it with tick option provided in email section before submit configuration form. the best demo way to test this section is create free mailtrap account where we get configurations.
this service should be enabled from common configuration file wherever this service act as dependency service. for example GPIO event system has event channel of email. GPIO events generated on user selected event channel.

* **GPIO Events:**
GPIO events are provided to get notified on specific condition met. from local web portal of device GPIO event conditions can be set in GPIO event section which is available under GPIO manage section.

* **Database:**
Database service can be used to get/set any config in device NVM memory. Schema model, the address-based table store, and the JSON-driven codegen are covered in [§6. Database Layer](#6-database-layer).

* **Auth Service:**
Auth service can be used to check whether user is logged in or not which required for few auth dependent services.

* **Serial Service:**
Serial service is mostly related to the serial communication of device. Serial service handles the serial events received on devices.

* **CommandLine Service:**
CommandLine service provides few basic commands to use. Full reference in [§10. Command Line / Terminal](#10-command-line--terminal).

* **Telnet Service:**
Telnet service accepts the client connections on default port 23. It will provide the terminal access through telent service. with this we can access device terminal remotely.

* **SSH Service:**
SSH service accepts the client connections on default port 22. It will provide the terminal access through SSH service. with this we can access device terminal remotely. Note that ssh key pair needs to be generated before using this service. use ssh command to generate ssh key pairs.

* **TLS (HTTPS + outbound TLS clients):**
When `ENABLE_TLS_SERVICE` is set, the framework's TCP client/server slot is replaced with a TLS-capable instance (BearSSL on esp8266, mbedTLS on esp32) so OTA, MQTT, Email, IoT and HTTP client calls travel encrypted with no service-side changes. `ENABLE_HTTPS_SERVER` similarly switches the web portal to port 443. Certs and keys live on the filesystem at `/etc/http/server.{crt,key}` (defaults overridable in [TlsConfig.h](src/config/TlsConfig.h)). esp32 builds can generate them on-device with the `tls` CLI command; esp8266 builds use [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) and SFTP. Full details in [§7.2.16](#7-service-providers).

# Local Web Server

PDI framework has local web portal. In case if device supports the internet then web server can be enabled. As an example Esp series devices has built in WiFi feature that work in both station as well as access point mode. Station mode is mode using which we can connect to other wifi network. Access point mode is mode using which Esp's create its own network. PDI stack comes with a local http server facility using access point mode of esp wifi. By default this server has setting, monitor pages added.

Web Server Framework has following components — the full request lifecycle, route registry, page composition, and "how to add a new page" walkthrough are in [§9. Web Server](#9-web-server).

* **Controllers :**
Controllers used to handle request from client. collecting user inputs, processing, building response for requests etc. works can be carried out in this component.

* **Middlewares :**
Middlewares used to provide filter like component for all requests. by default only auth middlware checks every request for its session. middlware needs to be assigned to route while registering them in controller.

* **Session Handler :**
Session handler takes care of login sessions. By default, login session expire after 5 minutes.

* **Route Handler :**
This handles routing operations of the server. It registers a specific controllers method to URL with facility of authentication through middlware.

* **Database :**
Devices can have non volatile memories where DB can store its data. Database service is used to store PDI framework related config data. Web Server uses this database service to fetch and view settings and control panel of device. For example Esp* has software eeprom library that actually uses space in flash memory

* **View Helpers :**
These helpers help to dynamically creates html elements.

* **Views :**
These are static html pages that split into header, middle and footer sections. At the time of sending http response we gathers them to form a complete html page.

* **Web Resources :**
These are required resources to server framework components.

# Features

Devices might have many built in features that will be useful in network applications. Those features are added with PDI Framework structure. The runtime cost of enabling each one — flash, RAM, per-session heap — is summarised in [§13.5 What each feature actually costs](#135-what-each-feature-actually-costs).

* **NAT :**
Network address translation (NAT) is a method of remapping one IP address space into another by modifying network address information in the IP header of packets while they are in transit across a traffic routing device. With this feature we can extend station network ( network that has active internet ) range.

    from v2.6.^ arduino has provided initial support example of NAT with lwip v2 variant (IPv4 only).

    before that lwip 1.4 is used to enable napt ( network address & port transform ) feature but with some customizations in lwip1.4.

    **you can test lwip 1.4** just rename "...esp8266/tools/sdk/lwip" with "...esp8266/tools/sdk/lwip.org" and copy lwip folder ( in this repo ) there. do not forget to select lwip 1.4 compile from source variant in arduino tools option while building.

    By default this feature is active based on what lwip variant from ide tool option is selected.

* **MESH :**
This feature easily possible with esp device esponow feature. in devices with esp* provided basic espnow service to make this available in application where mesh network is required. The basic motive to bring this feature is connectivity.


# Utilities

This common section is made to support/help all other section in their operations. This section consists of some most vital libraries that enable services to run in background/periodically. This section is the base for all other section hence all other services are dependent on this section. The complete utility inventory — scheduler, event bus, string ops, conversions, queues, crypto, PdiSTL — is documented in [§11. Utility Library](#11-utility-library).

* **Queue :**
Queue is dynamic service which enables users to push any data in it and pop it later for use.

* **String Helpers :**
String Helpers helps the user in many string related operations like finding, replacing, JSON parsing, etc.

* **Task Scheduler :**
Task Scheduler enables the feature of scheduling many things that executes later once or every time on specific intervals/timeouts. Scheduler also accepts priority, policy as parameter for task, where by default big number is kept as higher priority. Scheduler provides the api to change the scheduling mode where it can be inline, cooperative, preemptive task mode, once set the task mode the specific task will get scheduled accordingly if api supported to the device. make sure you understood the scheduling modes before apply them. By default all task are scheduled as inline tasks. The three modes, the four policies, and a decision tree for picking between them are detailed in [§5. Task Scheduler](#5-task-scheduler).

* **Reset Factory :**
This helps to reset the whole device to its default settings in case of device malfunctioning badly. By pressing flash key on device for about 6-7 seconds this service resets all settings to its default one. also this service accept task which should be run while reset factory executing.

* **Data Type Converters :**
As name clears the purpose of this utility. It just used to convert the data types from one to another like integer to string and vice versa.

* **Event :**
This is to handle specific event tasks that should be executed on event arrival. just register the event listener as task to perticular event and fire it when event happens.

* **CommandBase :**
This is to handle user commands. It will help to parse command options and provide it to respective command implementation to execute the command.

* **PdiStl :**
This is an low weight implementation of a C++ standard library for memory constrained devices imported from https://github.com/mike-matera/ArduinoSTL Thanks to mike for building this STL baseline for memory constrained devices. 

# Device Iot (beta)

Added example of device iot where internal services take care of publishing/sending payload provided by device to mqtt iot server. The service's contract — and why the application (not the device port) implements `iDeviceIotInterface` — is in [§7.2.9 DeviceIotServiceProvider](#729-deviceiotserviceprovider--__device_iot_service); the working sketch is walked through in [§15.6 DeviceIotExample](#156-deviceiotexample--implementing-ideviceiotinterface).
currently MQTT server configs for device iot are configured from server with DEVICE_IOT_CONFIG_REQ_URL set in **config/DeviceIotConfig.h**.
to DEVICE_IOT_CONFIG_REQ_URL http server should response the config includes
* topic
* password
* keepalive
* data rate
* samples per data

other configs like
* clientid, username are kept as device mac by default
* host, port can be configured in **config/DeviceIotConfig.h** file

you can modify them as per requirements

**Note** that currently mqtt configs (in **config/MqttConfig.h** OR in web portals) are not used for device iot purpose.

by default this service is disabled. to enable, just uncomment ENABLE_DEVICE_IOT in **devices/DeviceConfig.h** file

# Detailed Documentation

The sections below are an in-tree reference for contributors and porters. Each section is self-contained but cross-links to source paths so you can jump straight to the code.

**Table of contents**

1. [Architecture Overview](#1-architecture-overview)
2. [Device Layer & Porting Guide](#2-device-layer--porting-guide)
3. [Portable Interfaces](#3-portable-interfaces)
4. [Configuration System](#4-configuration-system)
5. [Task Scheduler](#5-task-scheduler)
6. [Database Layer](#6-database-layer)
7. [Service Providers](#7-service-providers)
8. [Transports](#8-transports)
9. [Web Server](#9-web-server)
10. [Command Line / Terminal](#10-command-line--terminal)
11. [Utility Library](#11-utility-library)
12. [Logger](#12-logger)
13. [Memory & Performance Notes](#13-memory--performance-notes)
14. [Build & Toolchain](#14-build--toolchain)
15. [Examples Walkthrough](#15-examples-walkthrough)
16. [Extending the Framework](#16-extending-the-framework)
17. [Troubleshooting & FAQ](#17-troubleshooting--faq)

---

## 1. Architecture Overview

PDI Framework is structured as a layered, ports-and-adapters stack. The lower layers describe **what** a device must be able to do (abstract interfaces); the upper layers describe **what the application offers** (services, transports, web server, CLI). The two are bridged at compile time by a single device-selection macro and a set of `ENABLE_*` feature flags.

### 1.1 Layered model

```
                       ┌────────────────────────────────────────┐
   Application sketch  │  examples/PdiStack/*.ino               │
                       └───────────────────┬────────────────────┘
                                           │ uses
                       ┌───────────────────▼────────────────────┐
   Orchestrator        │  PDIStack  (src/PdiStack.{h,cpp})      │
                       └───────────────────┬────────────────────┘
                                           │ wires up
   ┌───────────────────────────────────────┼───────────────────────────────────────┐
   │                                       │                                       │
   ▼                                       ▼                                       ▼
┌──────────────────┐         ┌──────────────────────────┐          ┌───────────────────────┐
│  Service Layer   │         │  Web Server / CLI / SSH  │          │  Transports           │
│  service_provider│◄────────┤  webserver/, cmd/, shell │◄─────────┤  transports/http,     │
│  /*ServiceProv.  │         │                          │          │  mqtt, smtp           │
└────────┬─────────┘         └──────────────┬───────────┘          └──────────┬────────────┘
         │                                  │                                 │
         │ all consume only                 │                                 │
         ▼                                  ▼                                 ▼
                       ┌──────────────────────────────────────────────────┐
   Utilities           │  src/utility/  (Scheduler, EventUtil, Database,  │
                       │  StringOps, CommandBase, crypto, pdistl, queue,  │
                       │  fiber, Base64, DataTypeConversions, ...)        │
                       └────────────────────────┬─────────────────────────┘
                                                │ + abstract contracts
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Interface Layer     │  src/interface/pdi/   (iDeviceControlInterface,  │
   (the "ports")       │  iClientInterface, iServerInterface, iWifi*,     │
                       │  iStorage*, iFileSystem*, iSerial*, iGpio*,      │
                       │  iLogger*, iDatabase*, iNtp*, iPing*, iWdt*,     │
                       │  threading: iExecution, iMutex, iCondvar, ...)   │
                       └────────────────────────┬─────────────────────────┘
                                                │ implemented by
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Device Layer        │  devices/esp32/, esp8266/, arduinouno/,          │
   (the "adapters")    │  mockdevice/   — concrete *Interface.cpp +       │
                       │  <name>_pdi.h aggregator                         │
                       └──────────────────────────────────────────────────┘
```

**Direction of dependency is strictly downward.** A service never includes a device header directly; it talks to an `i*Interface` pointer. The only place device-specific code is reached from above is the aggregator [src/interface/pdi.h](src/interface/pdi.h), which chooses *one* `<device>_pdi.h` based on the `DEVICE_*` macro emitted by `scripts/DeviceSetup.py` into [devices/DeviceSetup.h](devices/DeviceSetup.h).

### 1.2 The five layers

| Layer | Path | Role | Depends on |
|---|---|---|---|
| Application | `examples/` | The user's sketch — calls `PdiStack.initialize()` then `PdiStack.serve()` in `loop()` | Orchestrator |
| Orchestrator | [src/PdiStack.cpp](src/PdiStack.cpp), [src/PdiStack.h](src/PdiStack.h) | Single global `PDIStack PdiStack` that conditionally wires every enabled service | Services + Interfaces |
| Service Layer | [src/service_provider/](src/service_provider/) | One subclass of `ServiceProvider` per feature (WiFi, MQTT, OTA, GPIO, Auth, Cmd, …); registers itself in a global table | Utilities + Interfaces |
| Utilities | [src/utility/](src/utility/) | Reusable primitives with no device knowledge (scheduler, event bus, database engine, crypto, STL subset, fiber) | Interfaces only |
| Interface Layer | [src/interface/pdi/](src/interface/pdi/) | Pure abstract C++ ports (`i*Interface`) grouped into `drivers/`, `middlewares/`, `modules/`, `threading/` | — |
| Device Layer | [devices/](devices/) | Concrete adapters per MCU; each provides a `<name>_pdi.h` aggregator (e.g. [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)) that pulls in the relevant `*Interface.h` files for that board | Vendor SDK / Arduino core |

### 1.3 The `ServiceProvider` contract

Every feature service derives from [ServiceProvider](src/service_provider/ServiceProvider.h) and self-registers into a global table indexed by a `service_t` enum:

```cpp
ServiceProvider(service_t st, const char *_svc_name)
    : m_service_t(st), m_service_name(_svc_name), m_service_routine_task_id(-1) {
    m_services[st] = this;          // self-registration
}
```

The base class defines a uniform lifecycle that the orchestrator and CLI both rely on:

| Method | Purpose | Override? |
|---|---|---|
| `initService(void *arg)` | One-time setup; called from `PDIStack::initialize()` | Required |
| `stopService()` | Tear down; clears the service's scheduler task | Optional |
| `printConfigToTerminal(t)` | Used by the `srvc` CLI command | Optional |
| `printStatusToTerminal(t)` | Used by the `srvc` CLI command | Optional |
| `getService(st)` | Static lookup so services can find each other without `#include` cycles | — |

This is why the `srvc` terminal command can enumerate, configure and inspect any service generically — they all conform.

### 1.4 The interface contract (the "ports")

Interfaces are grouped by *role* rather than by feature:

| Group | Examples | Who implements | Who consumes |
|---|---|---|---|
| `drivers/` | `iGpioInterface`, `iWdtInterface` | Device adapter | GPIO service, framework core |
| `middlewares/` | `iClientInterface`, `iServerInterface`, `iNtpInterface`, `iPingInterface`, `iDeviceControlInterface`, `iUpgradeInterface` | Device adapter | OTA, MQTT, Email, HTTP, IoT services |
| `modules/serial`, `modules/storage`, `modules/wifi` | `iSerialInterface`, `iStorageInterface`, `iFileSystemInterface`, `iWiFiInterface`, `iHttpServerInterface` | Device adapter | Serial, Storage, WiFi services |
| `threading/` | `iExecution`, `iMutex`, `iCondvar`, `iContext` + `cooperative/`, `preemptive/` | Device adapter (if supported) | Task scheduler in contextual mode |
| Top-level | `iDatabaseInterface`, `iLoggerInterface`, `iDeviceIotInterface` | Device adapter (DB, logger) / framework (IoT) | Database service, logger macros, IoT service |

A device port is considered "complete" when it provides at least: `iDeviceControlInterface`, `iDatabaseInterface`, `iSerialInterface` (if `ENABLE_SERIAL_SERVICE`), and any group required by the feature flags it intends to support. See [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h) for the canonical aggregator pattern — note how each include is itself wrapped in the matching `ENABLE_*` guard so an unused interface costs zero code size.

### 1.5 Compile-time composition (feature gating)

There is **no runtime plugin system**. Every optional capability is selected at compile time through `ENABLE_*` macros declared in [devices/DeviceConfig.h](devices/DeviceConfig.h). Macros cascade: enabling `ENABLE_NETWORK_SERVICE` is what makes `ENABLE_MQTT_SERVICE`, `ENABLE_HTTP_SERVER`, `ENABLE_OTA_SERVICE`, etc. meaningful for that device. Dependencies are encoded as nested `#if` blocks; e.g. SSH is only enabled if storage is available:

```c
#ifdef ENABLE_NETWORK_SERVICE
  #define ENABLE_MQTT_SERVICE
  #define ENABLE_WIFI_SERVICE
  #if defined(ENABLE_STORAGE_SERVICE)
    #define ENABLE_SSH_SERVICE
  #endif
  ...
#endif
```

The same flag drives **three** independent things, in lockstep:

1. Whether the **device aggregator** includes the corresponding `*Interface.h` (see [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)).
2. Whether the **orchestrator** includes the corresponding `*ServiceProvider.h` (see [src/PdiStack.h](src/PdiStack.h)).
3. Whether `PDIStack::initialize()` calls the service's `initService(...)` (see [src/PdiStack.cpp](src/PdiStack.cpp)).

That triple-guard is intentional — it lets a feature be removed entirely from a build (zero flash, zero RAM, zero static-init cost) just by commenting one `#define`.

### 1.6 Runtime lifecycle

```
boot
 │
 ├─ static init of PDIStack PdiStack       (src/PdiStack.cpp)
 │     ├─ __utl_event.begin(&__i_dvc_ctrl)
 │     ├─ __task_scheduler.setUtilityInterface(...)
 │     └─ (if WiFi) m_client = __i_instance.getNewT(cp|ls)ClientInstance()
 │                  (TLS picked at compile time via ENABLE_TLS_SERVICE)
 │
setup()
 │
 ├─ PdiStack.initialize()                  (src/PdiStack.cpp)
 │     ├─ __i_dvc_ctrl.initDeviceSpecificFeatures()
 │     ├─ acquire terminal, attach to ServiceProvider base
 │     ├─ __database_service.initService()                (always)
 │     └─ each enabled service .initService(args)         (conditional)
 │
loop()
 │
 └─ PdiStack.serve()                        (src/PdiStack.cpp)
       ├─ __web_server.handle_clients()                    (if enabled)
       ├─ __task_scheduler.run()                           (inline tasks)
       ├─ __i_dvc_ctrl.yield()
       ├─ __i_dvc_ctrl.handleEvents()
       ├─ __i_cooperative_scheduler.tick_from_loop()       (if contextual)
       └─ __i_preemptive_scheduler.yield()                 (if contextual)
```

Two scheduling lanes coexist here:

- **Inline tasks** advance every `serve()` tick — best for periodic, short, non-blocking work.
- **Contextual tasks** (cooperative / preemptive) advance on their own stacks; the loop only *yields* to them. These exist only on devices whose port provides `iExecution` + a matching scheduler under [src/interface/pdi/threading/](src/interface/pdi/threading/).

Section [5. Task Scheduler] will cover the choice between modes in depth.

### 1.7 Global instances and naming

The framework exposes a small set of well-known global symbols, all prefixed with `__`. The naming is uniform so a contributor can grep for any one of them:

| Symbol | Provided by | Purpose |
|---|---|---|
| `__i_dvc_ctrl` | Device layer | The single `iDeviceControlInterface` for this build |
| `__i_db`, `__i_fs`, `__i_wifi`, `__i_http_server`, `__i_ntp`, `__i_ping`, `__i_serial` | Device layer | Interface singletons, only defined when the matching flag is on |
| `__task_scheduler`, `__utl_event` | Utility layer | Cross-service scheduler and event bus |
| `__database_service`, `__wifi_service`, `__mqtt_service`, `__ota_service`, ... | Service layer | One per `ServiceProvider` subclass |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | Threading port | Contextual execution lanes |
| `PdiStack` | Orchestrator | The single application-facing facade |

Services find each other by global symbol when the dependency is fixed (e.g. `__mqtt_service` reads from `__database_service`), and via `ServiceProvider::getService(st)` when a generic lookup is required (e.g. the `srvc` CLI command).

### 1.8 What this architecture buys you

- **Per-board minimisation** — a feature you don't enable is gone from the binary at all three layers.
- **Portability without polymorphism cost** — interfaces are concrete singletons selected at compile time; there is no vtable indirection on hot paths unless the port chooses it.
- **Clear porting surface** — adding a new device is a mechanical exercise: implement the required `i*Interface` classes under `devices/<name>/`, write a `<name>_pdi.h` aggregator, add a `DEVICE_<NAME>` branch to [src/interface/pdi.h](src/interface/pdi.h), and run `python3 scripts/DeviceSetup.py -d <name>`.
- **Service uniformity** — every service follows the same `init / stop / printConfig / printStatus` shape, which is why the CLI and web portal can manage them generically.

### 1.9 What it doesn't do (yet)

- No runtime service registration — adding a service requires editing `PDIStack` and the `service_t` enum.
- No automatic device detection — `DEVICE_*` must be set via the codegen script even though the Arduino core already exposes board macros. (Slated for [section 14. Build & Toolchain].)
- No DI container — service-to-service dependencies are global-symbol coupling, which is appropriate for embedded but limits unit-testability off-device.

---

## 2. Device Layer & Porting Guide

The device layer is the **only** place where vendor SDK / Arduino-core symbols are allowed. Everything above it talks to the device through abstract `i*Interface` pointers defined in [src/interface/pdi/](src/interface/pdi/). A port is the work of providing one folder under [devices/](devices/) that implements those interfaces for a specific MCU family.

### 2.1 Anatomy of a device folder

A complete port looks like this (using esp32 as the canonical example):

```
devices/esp32/
├── esp32.h                       SDK / Arduino-core umbrella include
├── esp32_device_config.h         Per-port platform macros (RODT_ATTR / PROG_RODT_ATTR / strcat_ro
│                                 / strcpy_ro family, CRITICAL_SECTION_ENTER/EXIT, etc.)
├── esp32_pdi.h                   Header aggregator — included by src/interface/pdi.h
├── esp32_pdi.cpp                 Source aggregator — see §2.4 on the "src-only" quirk
├── esp32_pdi.c                   (Optional) C-side aggregator for pure-C sources
├── DeviceControlInterface.{h,cpp}   Required — implements iDeviceControlInterface
├── DatabaseInterface.{h,cpp}        Required — implements iDatabaseInterface
├── LoggerInterface.{h,cpp}          Optional — only if logging is enabled
├── InstanceInterface.{h,cpp}        Required — factory for runtime instances
├── SerialInterface.{h,cpp}          If ENABLE_SERIAL_SERVICE
├── StorageInterface.{h,cpp}         If ENABLE_STORAGE_SERVICE
├── FileSystemInterface.{h,cpp}      If ENABLE_STORAGE_SERVICE
├── WiFiInterface.{h,cpp}            If ENABLE_WIFI_SERVICE
├── HttpServerInterface.{h,cpp}      If ENABLE_WIFI_SERVICE / HTTP_SERVER
├── TcpClientInterface.{h,cpp}       If ENABLE_NETWORK_SERVICE
├── TcpServerInterface.{h,cpp}       If ENABLE_NETWORK_SERVICE
├── NtpInterface.{h,cpp}             If ENABLE_NETWORK_SERVICE
├── PingInterface.{h,cpp}            If ENABLE_NETWORK_SERVICE
├── TlsClientInterface.{h,cpp}       If ENABLE_TLS_SERVICE  (esp8266 → BearSSL, esp32 → mbedTLS)
├── TlsServerInterface.{h,cpp}       If ENABLE_TLS_SERVICE
├── BearSSLCertLoader.{h,cpp}        esp8266 only: PEM/DER load helpers used by TlsClient/Server
│                                    (TlsCryptoLoader namespace)
├── MbedTLSCertLoader.{h,cpp}        esp32 only: same role, against mbedTLS
├── TlsCertProvisioner.{h,cpp}       esp32 only — if ENABLE_TLS_CERT_GENERATION
│                                    (self-signed EC/RSA cert issuance using mbedTLS)
├── ExceptionsNotifier.{h,cpp}       Optional — crash / exception capture
├── config/
│   └── DBTableSchema.json        Per-device DB schema; consumed by scripts
├── core/
│   └── EEPROM.{h,cpp}            Vendor-specific helpers (e.g. EEPROM emulator)
└── threading/                    Optional — only if the port provides cooperative
    │                              and/or preemptive execution contexts
    ├── Cooperative.{h,cpp}          iCooperativeScheduler implementation
    ├── CooperativeCondvar.{h,cpp}   iConditionVar pair for the cooperative lane
    ├── Preemptive.{h,cpp}           iPreemptiveScheduler implementation
    ├── PreemptiveMutex.{h,cpp}      iMutex implementation (IRQ-safe critical_lock pair)
    ├── XtensaContext.{h,cpp}        (esp8266) Xtensa register save/restore — iContext
    └── XtensaTimer.{h,cpp}          (esp8266) Hardware-timer ISR driving preemption
```

esp32 currently uses FreeRTOS task / mutex / semaphore primitives under the hood for its threading port (no `XtensaContext`/`XtensaTimer` files needed), while esp8266 ships the bare-metal Xtensa context save/restore + hardware-timer drive shown above.

Compare with the minimal end of the spectrum:

- [devices/mockdevice/](devices/mockdevice/) — header-only stub used when no `DEVICE_*` is defined; lets the framework compile for static analysis or off-device unit tests.
- [devices/arduinouno/](devices/arduinouno/) — no WiFi/Network/HTTP/Storage; ships only `DeviceControl`, `Database`, `Serial`, `Storage` (EEPROM-only), `FileSystem`, `Instance`, `Logger`.

### 2.2 Which interfaces are required vs optional

| Interface | Required? | Trigger flag | Notes |
|---|---|---|---|
| `iDeviceControlInterface` | **Always** | — | GPIO, reset, WDT, yield, events, terminal accessor |
| `iDatabaseInterface` | **Always** | — | NVM read/write for the config DB |
| `iInstanceInterface` | **Always** | — | Factory for fresh TCP client/server and FS handles |
| `iLoggerInterface` | Conditional | `ENABLE_LOG_*` | Skipped if no log level is enabled |
| `iSerialInterface` | Conditional | `ENABLE_SERIAL_SERVICE` | Powers the serial terminal |
| `iStorageInterface` + `iFileSystemInterface` | Conditional | `ENABLE_STORAGE_SERVICE` | LittleFS / SPIFFS / SD adapter |
| `iWiFiInterface`, `iHttpServerInterface` | Conditional | `ENABLE_WIFI_SERVICE` | WiFi STA+AP control, embedded HTTP server |
| `iTcpClientInterface`, `iTcpServerInterface` | Conditional | `ENABLE_NETWORK_SERVICE` | Raw TCP for MQTT/SMTP/OTA |
| `iNtpInterface`, `iPingInterface` | Conditional | `ENABLE_NETWORK_SERVICE` | Time sync, ICMP-based reachability |
| `iTlsClientInterface`, `iTlsServerInterface` | Conditional | `ENABLE_TLS_SERVICE` | TLS-wrapped TCP. Backend per port: BearSSL (esp8266), mbedTLS (esp32). When on, the orchestrator allocates these instead of plain TCP for `m_client`, so HTTP/MQTT/SMTP/OTA/IoT all upgrade to TLS transparently |
| `TlsCertProvisioner` (free functions, not a virtual interface) | Optional | `ENABLE_TLS_CERT_GENERATION` (esp32 only) | On-device self-signed cert/key issuance for the HTTPS server |
| `iExecution`, `iMutex`, `iCondvar`, `iContext` | Optional | `ENABLE_CONTEXTUAL_EXECUTION` | Needed for cooperative/preemptive task modes. Implicitly required by `ENABLE_TLS_SERVICE` so BearSSL/mbedTLS can run off the main loop's stack |
| `iGpioInterface`, `iWdtInterface` | Folded into `iDeviceControlInterface` | — | Implemented as part of `DeviceControl`, not separate classes today |

A port is "valid" the moment the **always** rows compile and link; every other row can be added incrementally as you turn on more `ENABLE_*` flags in [devices/DeviceConfig.h](devices/DeviceConfig.h).

### 2.3 The two aggregator files

Every port provides a pair of aggregators with a strict role split:

- **`<name>_pdi.h`** — declares which interface *headers* are visible to the rest of the framework. Each include is wrapped in the matching `ENABLE_*` guard so unused interfaces cost zero. See [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h).
- **`<name>_pdi.cpp`** — `#include`s the implementation `.cpp` files. **This is not a typo.** See §2.4 below.

Optionally:

- **`<name>_pdi.c`** — the same trick for C-only translation units.
- **`<name>.h`** — a small umbrella header pulling in the Arduino core / vendor SDK headers; included by every per-interface header so they don't each repeat the SDK plumbing.

### 2.4 Why `.cpp` files are `#include`d (the "src-only" quirk)

The Arduino IDE / `arduino-cli` build system only compiles `.cpp` / `.c` files that live **directly inside `<library>/src/`**. Anything under `<library>/devices/<name>/*.cpp` is ignored by the builder.

To work around this, each port ships a single compilable file at `<library>/src/` (in practice, the orchestrator's `PdiStack.cpp` and the per-device `<name>_pdi.cpp` reached transitively from there) which then `#include`s its sibling `.cpp` files:

```cpp
// devices/esp32/esp32_pdi.cpp
#include "esp32_pdi.h"
#include "DatabaseInterface.cpp"
#include "DeviceControlInterface.cpp"
#ifdef ENABLE_WIFI_SERVICE
  #include "WiFiInterface.cpp"
  #include "HttpServerInterface.cpp"
#endif
...
```

This produces a **single translation unit** per device, which has two practical consequences:

1. **Symbol uniqueness across `.cpp` files matters less** — anything you mark `static` is per-port, not per-file.
2. **Headers must still guard against double-include**, but `.cpp` files in the device folder must **not** be referenced from anywhere else (doing so would link them twice).

This is the dominant reason the framework cannot be installed unmodified via the Arduino Library Manager today — fixing it cleanly requires either moving sources under `src/` or switching to a build system that does recursive compilation. See [section 14. Build & Toolchain] for the full discussion.

### 2.5 Device selection flow

```
scripts/DeviceSetup.py -d esp8266
        │
        │  writes
        ▼
devices/DeviceSetup.h               #define DEVICE_ESP8266
        │
        │  #include'd by
        ▼
devices/DeviceConfig.h              cascades into ENABLE_* feature flags;
        │                           ALSO pulls in devices/<board>/<board>_device_config.h
        │                           via #if defined(DEVICE_*) → so RODT_ATTR, PROG_RODT_ATTR,
        │                           strcat_ro family, CRITICAL_SECTION_ENTER/EXIT
        │                           are defined before any framework header sees them
        │
        │  pulled in transitively from
        ▼
src/config/Config.h                 (everything in src/* sees the flags + platform macros)
        │
        ▼
src/interface/pdi.h                 selects <name>_pdi.h via #if defined(DEVICE_*)
        │
        ▼
devices/esp8266/esp8266_pdi.h       brings in the per-device interface headers
                                    (which transitively include esp8266.h for SDK symbols)
```

**Three** files need a matching entry when adding a device:
1. [devices/DeviceConfig.h](devices/DeviceConfig.h) — the `#if defined(DEVICE_*)` cascade at the top (to include `<board>_device_config.h`) **and** any per-device limits (`MAX_DIGITAL_GPIO_PINS`, `MAX_DB_TABLES`, etc.).
2. [src/interface/pdi.h](src/interface/pdi.h) — the `#if defined(DEVICE_*)` cascade that picks `<board>_pdi.h`.
3. [library.properties](library.properties) — add the architecture to the `architectures=` list ([§14.2](#14-build--toolchain)).

### 2.6 Per-device global singletons

Each port instantiates exactly one object per interface and names it according to the framework's convention. The rest of the framework refers to these names directly — they are part of the contract.

| Symbol | Defined in (esp32 example) | Required when |
|---|---|---|
| `__i_dvc_ctrl` | [DeviceControlInterface.cpp](devices/esp32/DeviceControlInterface.cpp) | Always |
| `__i_db` | [DatabaseInterface.cpp](devices/esp32/DatabaseInterface.cpp) | Always |
| `__i_instance` | [InstanceInterface.cpp](devices/esp32/InstanceInterface.cpp) | Always |
| `__i_logger` | [LoggerInterface.cpp](devices/esp32/LoggerInterface.cpp) | Any `ENABLE_LOG_*` |
| `__i_serial` | [SerialInterface.cpp](devices/esp32/SerialInterface.cpp) | `ENABLE_SERIAL_SERVICE` |
| `__i_storage`, `__i_fs` | [StorageInterface.cpp](devices/esp32/StorageInterface.cpp), [FileSystemInterface.cpp](devices/esp32/FileSystemInterface.cpp) | `ENABLE_STORAGE_SERVICE` |
| `__i_wifi`, `__i_http_server` | [WiFiInterface.cpp](devices/esp32/WiFiInterface.cpp), [HttpServerInterface.cpp](devices/esp32/HttpServerInterface.cpp) | `ENABLE_WIFI_SERVICE` |
| `__i_ntp`, `__i_ping` | [NtpInterface.cpp](devices/esp32/NtpInterface.cpp), [PingInterface.cpp](devices/esp32/PingInterface.cpp) | `ENABLE_NETWORK_SERVICE` |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | [devices/esp8266/threading/](devices/esp8266/threading/), [devices/esp32/threading/](devices/esp32/threading/) | `ENABLE_CONTEXTUAL_EXECUTION` |

If your port skips a flag, also skip the symbol — it must not exist when the corresponding `ENABLE_*` is off.

### 2.7 The optional threading port

Cooperative / preemptive task modes require five additional pieces, implemented under [devices/esp8266/threading/](devices/esp8266/threading/) and [devices/esp32/threading/](devices/esp32/threading/):

- An `iExecution`-derived scheduler (one for cooperative, one for preemptive).
- An `iContext` implementation that saves/restores CPU state (e.g. `XtensaContext` on esp8266; FreeRTOS task primitives on esp32).
- An `iMutex` and `iCondvar` pair appropriate for the mode.
- A periodic tick source for the preemptive lane (e.g. `XtensaTimer` on esp8266; FreeRTOS scheduler tick on esp32).

If a port omits this layer, contextual tasks are simply unavailable — inline tasks continue to work because they share the `loop()` stack. Section [5. Task Scheduler](#5-task-scheduler) details the API and the trade-offs.

`ENABLE_TLS_SERVICE` implicitly turns this layer on — BearSSL (esp8266) and mbedTLS (esp32) handshakes overflow the default Arduino main stack, so the TLS work runs on a dedicated cooperative task sized by `TLS_TASK_STACK_SIZE`.

### 2.8 Per-device database schema

Each port carries its own [config/DBTableSchema.json](devices/esp32/config/DBTableSchema.json) describing the tables present in NVM for that board (capacity-tuned). `scripts/DeviceSetup.py` invokes `CreateDBSourceFromJson.py` against this file to emit C++ table sources under [src/database/tables/](src/database/tables/). See section [6. Database Layer] for the schema format.

### 2.9 Adding a new device — step by step

Suppose you are porting to a board called `myboard`.

1. **Create the folder and umbrella headers**
   ```
   devices/myboard/
   ├── myboard.h                  // pulls in Arduino.h / vendor SDK
   ├── myboard_device_config.h    // per-port platform macros: RODT_ATTR, PROG_RODT_ATTR,
   │                              // strcat_ro / strcpy_ro family, CRITICAL_SECTION_ENTER/EXIT
   └── config/DBTableSchema.json  // start from devices/esp32/config/DBTableSchema.json
   ```
   Wire `myboard_device_config.h` into the device-config cascade by adding a `#elif defined(DEVICE_MYBOARD)` branch in [devices/DeviceConfig.h](devices/DeviceConfig.h) alongside the existing esp8266/esp32/arduinouno includes.
2. **Implement the always-required interfaces**: `DeviceControlInterface`, `DatabaseInterface`, `InstanceInterface`. Each `.h` derives from the matching `i*Interface` in [src/interface/pdi/](src/interface/pdi/); each `.cpp` defines the global singleton with the canonical `__i_*` name.
3. **Write the two aggregators**
   - `myboard_pdi.h` — mirror [esp32_pdi.h](devices/esp32/esp32_pdi.h), keeping only the interfaces you've implemented and guarding each with the right `ENABLE_*`.
   - `myboard_pdi.cpp` — mirror [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp), `#include`ing the corresponding `.cpp` files.
4. **Register the device in the selector**
   ```cpp
   // src/interface/pdi.h
   #elif defined(DEVICE_MYBOARD)
   #include "../../devices/myboard/myboard_pdi.h"
   ```
5. **Add per-device limits** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (`MAX_DIGITAL_GPIO_PINS`, `MAX_ANALOG_GPIO_PINS`, `MAX_DB_TABLES`, and gate any service flags the board cannot support, mirroring the `DEVICE_ARDUINOUNO` pattern).
6. **Run the setup script** from `scripts/`:
   ```bash
   python3 DeviceSetup.py -d myboard
   ```
   This writes `devices/DeviceSetup.h` with `#define DEVICE_MYBOARD` and generates the DB table sources.
7. **Build the bundled example** ([examples/PdiStack/](examples/PdiStack/)) for the new board to validate.
8. **Iterate on optional interfaces** (network, storage, WiFi, …), enabling the matching `ENABLE_*` in `DeviceConfig.h` one at a time and adding the corresponding `*Interface.{h,cpp}` files.
9. **(Optional) Add TLS support** by implementing `TlsClientInterface` / `TlsServerInterface` against your board's preferred SSL stack (e.g. mbedTLS via ESP-IDF, BearSSL on smaller boards) plus a cert-loader namespace (`TlsCryptoLoader`) for PEM/DER ingestion from the FS. Wire `__i_instance.getNewTlsClient/ServerInstance()` to allocate them. See [devices/esp8266/TlsClientInterface.h](devices/esp8266/TlsClientInterface.h) and [devices/esp32/TlsClientInterface.h](devices/esp32/TlsClientInterface.h) as references — both implement the same `iTlsClientInterface` contract on different SSL stacks.
10. **(Optional) Add the threading port** under `devices/myboard/threading/` if you want `ENABLE_CONTEXTUAL_EXECUTION` (or you've turned on TLS, which requires it). Provide `Cooperative` / `Preemptive` schedulers + matching `Mutex` / `Condvar`. The two existing ports — bare-metal Xtensa context on esp8266, FreeRTOS-backed on esp32 — show both ends of the implementation spectrum.

### 2.10 Validation checklist

A port is ready to merge when:

- [ ] `devices/mockdevice/` still compiles (you didn't accidentally couple `src/` to a vendor header).
- [ ] The bundled `examples/PdiStack` builds with all flags this board *can* support enabled.
- [ ] Every `__i_*` symbol expected by the build's `ENABLE_*` set is defined exactly once.
- [ ] The `srvc l` CLI command lists every service your build started.
- [ ] `reboot` and factory-reset both round-trip without losing the DB.
- [ ] If `ENABLE_STORAGE_SERVICE` is on, SFTP upload/download via `scp` works.
- [ ] If `ENABLE_CONTEXTUAL_EXECUTION` is on, a sample task scheduled under `__i_cooperative_scheduler` and `__i_preemptive_scheduler` prints from both lanes without stack corruption.
- [ ] If `ENABLE_TLS_SERVICE` is on, `__i_instance.getNewTlsClientInstance()` returns a non-null instance and a manual TLS handshake against a known peer (e.g. `curl --resolve` to the device, or outbound to a public HTTPS endpoint) completes without panic.
- [ ] If `ENABLE_HTTPS_SERVER` is on, the portal answers on port 443 once `/etc/http/server.{crt,key}` are present.
- [ ] If `ENABLE_TLS_CERT_GENERATION` is on (esp32 only), `tls q=1,t=0,l=256,n=test,i=1` writes both files to the configured paths.

---

## 3. Portable Interfaces

The interface layer is the **contract** between the framework and any device. Every type is an abstract C++ class with pure-virtual methods, no state, and no platform headers — it depends only on standard types, the framework's own [pdistl](src/utility/pdistl/) primitives, and other interfaces in this layer. Each one is implemented by exactly one device-side class per build (selected at compile time) and surfaced as a single global `__i_*` symbol.

### 3.1 Layout

```
src/interface/pdi/
├── iDatabaseInterface.h       NVM-backed key/blob store
├── iLoggerInterface.h         Structured log sink
├── iDeviceIotInterface.h      Application hook for the IoT service
├── drivers/                   "Bare metal" surfaces
│   ├── iGpioInterface.h           Digital/analog/blink GPIO
│   └── iWdtInterface.h            Watchdog
├── middlewares/               Higher-level building blocks
│   ├── iDeviceControlInterface.h  Composite (gpio + wdt + util + upgrade + …)
│   ├── iClientInterface.h         Generic stream client + iTcpClientInterface
│   │                              + iTlsClientInterface (ENABLE_TLS_SERVICE)
│   ├── iServerInterface.h         iTcpServerInterface + iTlsServerInterface
│   │                              + iHttpServerInterface (with HTTPS hooks)
│   ├── iNtpInterface.h            Time sync
│   ├── iPingInterface.h           Reachability check
│   └── iUpgradeInterface.h        OTA primitive
├── modules/                   Stand-alone feature surfaces
│   ├── serial/iSerialInterface.h
│   ├── storage/iStorageInterface.h        Raw byte-addressable backing store
│   ├── storage/iFileSystemInterface.h     Files & directories on top of storage
│   └── wifi/iWiFiInterface.h
├── threading/                 Optional execution-context surfaces
│   ├── iContext.h, iMutex.h, iCondvar.h, iExecution.h
│   ├── cooperative/iCooperative.h
│   └── preemptive/iPreemptive.h
└── impl/                      Default implementations reusable across ports
    ├── middlewares/HttpServerInterfaceImpl.{h,cpp}
    └── modules/storage/FileSystemInterfaceImpl.{h,c,cpp}
```

In addition, [src/utility/](src/utility/) ships three foundational interfaces that the `pdi/` headers build on — they live with the utilities because they have no device dependency at all:

| Foundation interface | Path | Role |
|---|---|---|
| `iIOInterface` / `iTerminalInterface` | [src/utility/iIOInterface.h](src/utility/iIOInterface.h) | Byte/line I/O contract — base of every stream-like interface (serial, TCP client, terminal sessions) |
| `iUtilityInterface` | [src/utility/iUtilityInterface.h](src/utility/iUtilityInterface.h) | `wait`, `millis_now`, `yield`, `log`, optional stack measurement |
| `iInstanceInterface` | [src/utility/iInstanceInterface.h](src/utility/iInstanceInterface.h) | Factory: new TCP client/server, get utility/filesystem |

### 3.2 Naming and discovery conventions

- All interface types are prefixed with a lowercase `i` (`iWiFiInterface`, `iClientInterface`).
- Each interface header forward-declares the **concrete** derived class with the canonical name (`class DeviceControlInterface;`) and at the bottom declares the singleton:
  ```cpp
  extern DeviceControlInterface __i_dvc_ctrl;
  ```
  This is what couples a port to the framework's expected symbol — the device's `.cpp` defines the matching variable.
- Composite interfaces use multiple inheritance to bundle smaller contracts; see [`iDeviceControlInterface`](src/interface/pdi/middlewares/iDeviceControlInterface.h) which inherits from `iGpioInterface`, `iWdtInterface`, `iUtilityInterface`, and `iUpgradeInterface`.
- Stream-like things (serial, TCP, terminal sessions) all share `iTerminalInterface` as a base, so the terminal, logger, and CLI can write to any of them uniformly.

### 3.3 Interface reference

Each row below: what the interface models, who implements it on a typical port, who consumes it from above.

#### 3.3.1 Core (always required)

| Interface | Path | Implementer | Primary consumers | Key methods |
|---|---|---|---|---|
| `iDeviceControlInterface` | [middlewares/iDeviceControlInterface.h](src/interface/pdi/middlewares/iDeviceControlInterface.h) | Device | `PDIStack`, every service via `__i_dvc_ctrl` | `initDeviceSpecificFeatures`, `resetDevice`, `restartDevice`, `eraseConfig`, `getDeviceId`, `getDeviceMac`, `isDeviceFactoryRequested`, `getTerminal`, `handleEvents` (+ inherited GPIO/WDT/utility/upgrade) |
| `iDatabaseInterface` | [iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h) | Device | `DatabaseServiceProvider`, every config table | `beginConfigs(size)`, `cleanAllConfigs`, `isValidConfigs`, `getMaxDBSize`, plus templated typed read/write |
| `iInstanceInterface` | [src/utility/iInstanceInterface.h](src/utility/iInstanceInterface.h) | Device | Services that need fresh TCP/TLS/FS instances (MQTT pool, SSH, OTA, HTTPS) | `getNewTcpClientInstance`, `getNewTcpServerInstance`, `getNewTlsClientInstance` / `getNewTlsServerInstance` (`ENABLE_TLS_SERVICE`), `getFileSystemInstance`, `getUtilityInstance` |
| `iUtilityInterface` | [src/utility/iUtilityInterface.h](src/utility/iUtilityInterface.h) | Inherited via `iDeviceControlInterface` | Scheduler, event bus, logger | `wait`, `millis_now`, `yield`, `log`, optional `can_measure_stack` / `measure_lastfn_stack` |
| `iIOInterface`, `iTerminalInterface` | [src/utility/iIOInterface.h](src/utility/iIOInterface.h) | Any stream (serial, TCP, etc.) | Logger, CLI, web body writers | `write`/`writeln` family (overloaded for all primitive types + `RODT_ATTR` strings), `with_timestamp`, `connect`/`disconnect` |

#### 3.3.2 Drivers

| Interface | Path | Implementer | Consumers | Key methods |
|---|---|---|---|---|
| `iGpioInterface` | [drivers/iGpioInterface.h](src/interface/pdi/drivers/iGpioInterface.h) | Device (folded into `DeviceControlInterface`) | `GpioServiceProvider`, `gpio` CLI | `gpioMode`, `gpioWrite`, `gpioRead`, `gpioFromPinMap`, `isExceptionalGpio`, `createGpioBlinkerInstance` / `releaseGpioBlinkerInstance` |
| `iGpioBlinkerInterface` | same file | Device | GPIO service for blink mode | `setConfig`, `updateConfig`, `start`, `stop`, `isRunning` |
| `iWdtInterface` | [drivers/iWdtInterface.h](src/interface/pdi/drivers/iWdtInterface.h) | Device (folded into `DeviceControlInterface`) | Long-running services, scheduler | `enableWdt(mode)`, `disableWdt`, `feedWdt` |

#### 3.3.3 Middlewares (networking & device-level operations)

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iClientInterface` | [middlewares/iClientInterface.h](src/interface/pdi/middlewares/iClientInterface.h) | Serial, TCP, telnet/SSH sessions | Terminal sessions, transports | Adds only `setTimeout` on top of `iTerminalInterface` — the I/O surface |
| `iTcpClientInterface` | same file | Device | OTA, MQTT, SMTP, HTTP client | Adds `getLocalIp`, `getRemoteIp`, ports, `setKeepAlive`, `setNoDelay` |
| `iTlsClientInterface` | same file | Device (esp8266 via BearSSL, esp32 via mbedTLS) | Same consumers as TCP when `ENABLE_TLS_SERVICE` is on — orchestrator allocates this instead of plain TCP, see [src/PdiStack.cpp](src/PdiStack.cpp) | Extends `iTcpClientInterface`: `setCertificateAuthorityPath`, `setClientCertificatePath`, `setClientPrivateKeyPath`, `setSNIHostname`, `setVerifyPeer`, `isSecure()` |
| `iTcpServerInterface` | [middlewares/iServerInterface.h](src/interface/pdi/middlewares/iServerInterface.h) | Device | Telnet, SSH, raw TCP services | `begin(port)`, `hasClient`, `accept`, `close` |
| `iTlsServerInterface` | same file | Device | HTTPS server, future TLS-wrapped Telnet/MQTT brokers | Extends `iTcpServerInterface`: `setServerCertificatePath`, `setServerPrivateKeyPath`, `setClientCertificateAuthorityPath` (mTLS). `accept()` returns a TLS-capable client transparently |
| `iHttpServerInterface` | same file | Device (or `impl/HttpServerInterfaceImpl`) | `WebServer` | Routing (`on`, `onNotFound`), args/headers, `send(code, mime, body, chunked)`. With TLS: `begin(port, secure=true)` plus `setServerCertificatePath` / `setServerPrivateKeyPath` / `setClientCertificateAuthorityPath` |
| `iNtpInterface` | [middlewares/iNtpInterface.h](src/interface/pdi/middlewares/iNtpInterface.h) | Device | Logger timestamps, IoT, sessions | `init_ntp_time`, `is_valid_ntptime`, `get_ntp_time` |
| `iPingInterface` | [middlewares/iPingInterface.h](src/interface/pdi/middlewares/iPingInterface.h) | Device | WiFi service (internet-availability check) | `init_ping(wifi)`, `ping`, `isHostRespondingToPing` |
| `iUpgradeInterface` | [middlewares/iUpgradeInterface.h](src/interface/pdi/middlewares/iUpgradeInterface.h) | Device (folded into `DeviceControlInterface`) | `OtaServiceProvider` | `Upgrade(path, version) → upgrade_status_t` |

#### 3.3.4 Modules

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iSerialInterface` | [modules/serial/iSerialInterface.h](src/interface/pdi/modules/serial/iSerialInterface.h) | Device | `SerialServiceProvider`, logger, CLI | Derives from `iClientInterface`; serial is just another stream |
| `iStorageInterface` | [modules/storage/iStorageInterface.h](src/interface/pdi/modules/storage/iStorageInterface.h) | Device (EEPROM / flash / SD adapter) | `iFileSystemInterface`, `littlefs` | Byte-addressable: `read`, `write`, `erase`, `size` |
| `iFileSystemInterface` | [modules/storage/iFileSystemInterface.h](src/interface/pdi/modules/storage/iFileSystemInterface.h) | Device (often via [`FileSystemInterfaceImpl`](src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h)) | SSH/SFTP, `cat`/`fwrite`/`ls`/`cd`/`mv`/`cp`/`rm`/`mkd`/`mkf`/`head`/`tail`/`wc`/`df`/`grep`/`hexdump` CLI | 42 virtuals: file & directory CRUD, traversal, line/offset lookup, search (`findInFile`, `getLineNumbersInFile`, `getOffsetFromLineNumber`, `getLineNumberFromOffset`), and custom file attributes (`setFileAttr` / `getFileAttr` / `removeFileAttr`). Constructed with an `iStorageInterface&` |
| `iWiFiInterface` | [modules/wifi/iWiFiInterface.h](src/interface/pdi/modules/wifi/iWiFiInterface.h) | Device (esp8266/esp32) | `WiFiServiceProvider`, `net` CLI | STA + AP, scan (sync/async), `enableNAPT`, `setMode`/`getMode` |

#### 3.3.5 Optional helpers

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iLoggerInterface` | [iLoggerInterface.h](src/interface/pdi/iLoggerInterface.h) | Device | `LOG*` macros | `init`, `log(type, msg)`, type-specific helpers, `log_format(fmt, type, …)` |
| `iDeviceIotInterface` | [iDeviceIotInterface.h](src/interface/pdi/iDeviceIotInterface.h) | **Application** (not device) | `DeviceIotServiceProvider` | `sampleHook`, `dataHook(payload)`, `resetSampleHook` — implemented in the user's sketch to feed IoT payloads |

> Note: `iDeviceIotInterface` is the only `i*Interface` whose implementer is the *application*, not the device port. It is the framework's intentional extension point for "what to publish, on what schedule".

#### 3.3.6 Threading (only required for contextual execution)

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iContext` | [threading/iContext.h](src/interface/pdi/threading/iContext.h) | Device (CPU-specific) | Cooperative + preemptive schedulers | `save(out)`, `restore(in)` — raw register save/restore |
| `iMutex` | [threading/iMutex.h](src/interface/pdi/threading/iMutex.h) | Device | Anything sharing state across contexts | `lock`/`unlock` + `critical_lock`/`critical_unlock` (IRQ-safe) |
| `iConditionVar` | [threading/iCondvar.h](src/interface/pdi/threading/iCondvar.h) | Device | Cooperative scheduler primitives | `wait(mtx)`, `notify_one`, `notify_all` |
| `iExecutionContext` | [threading/iExecution.h](src/interface/pdi/threading/iExecution.h) | Device | Schedulers | `start`, `suspend`, `resume` |
| `iExecutive` | same file | Device | Tasks | Carries stack pointer/size + entry/arg + back-link to `task_t` |
| `iExecutionScheduler` | same file | Device | `TaskScheduler` | `schedule_task`, `mute`, `yield`, `sleep`, `run`, optional `enable_sched`/`disable_sched` and cross-scheduler hand-off |
| `iCooperative`, `iCooperativeScheduler` | [threading/cooperative/iCooperative.h](src/interface/pdi/threading/cooperative/iCooperative.h) | Device | Cooperative-mode tasks | Specialisation tags |
| `iPreemptive`, `iPreemptiveScheduler` | [threading/preemptive/iPreemptive.h](src/interface/pdi/threading/preemptive/iPreemptive.h) | Device | Preemptive-mode tasks | Specialisation tags |

The `SoftIrq` machinery in [iExecution.h](src/interface/pdi/threading/iExecution.h) lives in the interface header because it is the shared protocol between any ISR (device side) and the scheduler (utility side): the ISR sets a bit via `raise_softirq()`, the main context drains it with `fetch_softirq_bits()`. Ports that need preemption must wire their tick ISR to this protocol.

### 3.4 The `impl/` directory — shared default implementations

Not every interface is best re-written per device. [src/interface/pdi/impl/](src/interface/pdi/impl/) holds **portable default implementations** that a device can adopt as-is:

| Default impl | What it gives you | When a port should override |
|---|---|---|
| [`HttpServerInterfaceImpl`](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h) | A protocol-correct HTTP/1.1 server built on top of `iTcpServerInterface` + `iTcpClientInterface`. When `ENABLE_TLS_SERVICE` is on, `begin(port, secure=true)` wraps accepted connections via `__i_instance.getNewTlsServerInstance()` so the same impl serves HTTPS without a separate file | Almost never — only if the SDK provides a measurably better native HTTP server |
| [`FileSystemInterfaceImpl`](src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h) | LittleFS-based filesystem on top of any `iStorageInterface` | Only if the SDK exposes its own FS more efficiently |

This is how the framework keeps the porting effort small: a new device only needs raw `iTcpServerInterface` + `iStorageInterface`, and the rest of the stack (HTTP server, HTTPS server, file system) is inherited. The TLS client/server classes are **not** in `impl/` because BearSSL vs. mbedTLS are fundamentally different libraries — each port supplies its own pair from scratch.

### 3.5 Lifecycle and threading expectations

Every interface implementation must respect three contracts:

1. **Construction is cheap and side-effect-free.** Singletons are constructed before `setup()` — they must not allocate from the heap, touch hardware, or open ports in their constructor. All real work belongs in an `init*` method invoked from `PDIStack::initialize()` (or from another service's `initService`).
2. **Methods are non-blocking unless explicitly otherwise.** Anything that could take more than a few milliseconds (TCP connect, NTP fetch, OTA download) must return early or expose a state-machine; long-form work is driven by the scheduler, not held inside the interface call.
3. **No re-entrancy assumed.** The framework runs single-threaded inline by default. If the port enables contextual execution, the *implementation* (not the caller) is responsible for guarding shared state with `iMutex` — interfaces document where this matters per-method.

### 3.6 Adding a new interface

The bar for adding an interface is "is there at least one device that can implement it differently?". If yes:

1. Decide the group: `drivers/` (silicon-level), `middlewares/` (network/device), `modules/` (orthogonal feature), `threading/` (execution), or top-level (cross-cutting like database/logger).
2. Forward-declare the concrete class, declare `extern Concrete __i_<name>;` at the bottom.
3. Add a guard so no existing port has to provide it until it opts in — usually via the same `ENABLE_*` flag that gates the consuming service.
4. Provide a stub in [devices/mockdevice/](devices/mockdevice/) so the off-device build still links.
5. Document required vs optional methods in this section.

A new interface that ships with only one device implementation is a smell — collapse it into the device-specific code until a second port appears.

---

## 4. Configuration System

Configuration in PDI Framework is **layered, compile-time, and additive**. There is no `.ini`, no runtime config parser, and no over-the-air feature toggling. Every choice — which services exist, how large each table is, what the defaults are, which interfaces a port supplies — is fixed when you build.

This section is the canonical reference for: every `ENABLE_*` flag and what it pulls in, the per-service `*Config.h` files and the structs they declare for the database, and the dependency relationships you must respect when turning features on or off.

### 4.1 Three tiers of configuration

The configuration system has three distinct tiers, each addressing a different concern:

| Tier | Lives in | Owns | Audience |
|---|---|---|---|
| **Device tier** | [devices/DeviceSetup.h](devices/DeviceSetup.h) (autogen) + [devices/DeviceConfig.h](devices/DeviceConfig.h) + each port's `devices/<board>/<board>_device_config.h` | `DEVICE_*` selector (DeviceSetup), `ENABLE_*` feature flags + per-board limits (DeviceConfig), per-port platform macros — `RODT_ATTR`, `PROG_RODT_ATTR`, `strcat_ro` family, `CRITICAL_SECTION_ENTER/EXIT` (`<board>_device_config.h`, included by DeviceConfig.h based on `DEVICE_*`) | Integrator: "what board, what features"; porter: "how the board names flash / interrupts" |
| **Common tier** | [src/config/Common.h](src/config/Common.h), [src/config/GlobalConfig.h](src/config/GlobalConfig.h) | Cross-cutting constants (`MAX_SCHEDULABLE_TASKS`, default user/pass, base time durations), the always-present `global_config` table | Framework author + integrator |
| **Service tier** | One `*Config.h` per service under [src/config/](src/config/) | Per-service tuning knobs **and** the runtime config struct(s) persisted to NVM | Service author + advanced integrator |

The tiers are pulled in via the single entry point [src/config/Config.h](src/config/Config.h), which conditionally includes each service-tier header based on the `ENABLE_*` flags decided by the device tier. Every translation unit in the framework reaches the configuration system through this one header.

### 4.2 Inclusion graph

```
devices/DeviceSetup.h              (autogenerated: #define DEVICE_<NAME>)
        │
        ▼
devices/DeviceConfig.h             (cascades DEVICE_* into ENABLE_* flags;
        │                           also pulls in devices/<board>/<board>_device_config.h
        │                           for per-port platform macros — RODT_ATTR,
        │                           PROG_RODT_ATTR, strcat_ro family,
        │                           CRITICAL_SECTION_ENTER/EXIT)
        ▼
src/config/Common.h                (defaults that other configs reference)
        │
        ▼
src/config/GlobalConfig.h          (the always-loaded `global_config` table)
        │
        ▼
src/config/Config.h                ── conditional includes ──┐
                                                              │
   ┌────────────────────┬───────────────────┬─────────────────┼─────────────────┐
   ▼                    ▼                   ▼                 ▼                 ▼
WifiConfig.h     ServerConfig.h     HttpConfig.h        StorageConfig.h    OtaConfig.h
                                          ⋮                  ⋮                 ⋮
                              (one per ENABLE_*-gated service)
```

Two implications:

1. A `*Config.h` for a disabled service is **never** included — so the struct it declares does not exist in the build, and the table address it would occupy is reclaimed.
2. Every config file may freely include `Common.h` for shared defaults, but **must not** include other service configs — that would couple two services through configuration. The orchestrator owns ordering.

### 4.3 Device-tier flags — full reference

Every `ENABLE_*` (and `ALLOW_*` / `IGNORE_*` / `AUTO_*`) flag lives in [devices/DeviceConfig.h](devices/DeviceConfig.h) and acts as a triple gate: it controls (1) which interface a device exposes, (2) which service the orchestrator includes, and (3) which `*Config.h` is pulled into `Config.h`.

#### 4.3.1 Service flags

| Flag | Default on | Brings in | Requires | Memory cost (approx) |
|---|---|---|---|---|
| `ENABLE_SERIAL_SERVICE` | yes (all boards) | SerialServiceProvider + `SerialConfig.h` | `iSerialInterface` | low |
| `ENABLE_STORAGE_SERVICE` | yes (default; auto-disabled on UNO) | `StorageConfig.h` + filesystem | `iStorageInterface` + `iFileSystemInterface` | medium (FS overhead) |
| `ENABLE_GPIO_SERVICE` | yes | GpioServiceProvider + `GpioConfig.h` | `iGpioInterface` (via `iDeviceControlInterface`) | low; `ENABLE_GPIO_BASIC_ONLY` trims it further for uno |
| `ENABLE_CMD_SERVICE` | yes | CommandLineServiceProvider | A terminal source (serial / telnet / ssh) | low |
| `ENABLE_AUTH_SERVICE` | non-uno | AuthServiceProvider + `ServerConfig.h` (`login_credential` table) | Storage | low |
| `ENABLE_NETWORK_SERVICE` | non-uno | Umbrella — gates everything below | `iTcpClient/Server`, `iNtp`, `iPing`, `iWiFi` | — |
| `ENABLE_WIFI_SERVICE` | with network | WiFiServiceProvider + `WifiConfig.h` | `iWiFiInterface` | medium |
| `ENABLE_HTTP_SERVER` | with network | WebServer + `ServerConfig.h` + `HttpConfig.h` | `iHttpServerInterface` (or `HttpServerInterfaceImpl`) | medium |
| `ENABLE_HTTPS_SERVER` | off; with `TLS_SERVICE` | Switches `WebServer::initService` to `begin(HTTPS_DEFAULT_PORT=443, secure=true)` with cert/key paths from `TlsConfig.h` | `ENABLE_TLS_SERVICE` (+ optional `ENABLE_HTTPS_SERVER_MTLS` for client-cert auth) | high (TLS handshake + per-session buffers) |
| `ENABLE_HTTP_CLIENT` | with network | HttpHelper + `HttpConfig.h` | `iTcpClientInterface` | low |
| `ENABLE_MQTT_SERVICE` | with network | MqttServiceProvider + `MqttConfig.h` | TCP client | medium |
| `ENABLE_OTA_SERVICE` | with network | OtaServiceProvider + `OtaConfig.h` | TCP client + `iUpgradeInterface` | low |
| `ENABLE_EMAIL_SERVICE` | with network | EmailServiceProvider + `EmailConfig.h` | TCP client | low |
| `ENABLE_TELNET_SERVICE` | with network | TelnetServiceProvider | TCP server | low |
| `ENABLE_SSH_SERVICE` | with network **and** storage | SSHServiceProvider | TCP server + FS | **high** (keys, crypto, SFTP) |
| `ENABLE_DEVICE_IOT` | with network | DeviceIotServiceProvider + `DeviceIotConfig.h` | TCP client | low (app supplies hooks) |
| `ENABLE_TLS_SERVICE` | off | TLS-capable client/server (`iTlsClientInterface` / `iTlsServerInterface`) + `TlsConfig.h`; forces `ENABLE_CONTEXTUAL_EXECUTION` on as TLS runs on a dedicated cooperative task | esp8266 (BearSSL) or esp32 (mbedTLS) backend in the port | **high** — see [§13.5](#135-what-each-feature-actually-costs) |
| `ENABLE_TLS_CERT_GENERATION` | off (esp32 only) | `tls` CLI command + `TlsCertProvisioner` (mbedTLS issuer) | `ENABLE_TLS_SERVICE` + esp32 | medium (mbedTLS keygen frames) |
| `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` | off | Listener on `EVENT_WIFI_STA_GOT_IP` that auto-mints a self-signed cert for the device's IP if `/etc/http/server.crt` is missing | `ENABLE_TLS_CERT_GENERATION` | low at idle, one-shot keygen cost on first boot |
| `ENABLE_CONTEXTUAL_EXECUTION` | off (esp8266 + esp32) | Cooperative + preemptive scheduler lanes | `iExecution`, `iMutex`, `iCondvar`, `iContext` + scheduler globals | high (per-task stacks) |
| `ENABLE_TIMER_TASK_SCHEDULER` | off (removed from default header) | Timer-backed task scheduler variant | Device timer | depends |

#### 4.3.2 Behaviour flags

| Flag | Effect |
|---|---|
| `ENABLE_GPIO_BASIC_ONLY` | Reduces GPIO service to digital-only (used on uno) |
| `ENABLE_DYNAMIC_SUBNETTING` | AP subnet/gateway picked at runtime instead of static |
| `ENABLE_NAPT` | Network address & port translation; lets AP clients reach the STA's network. Increases heap |
| `IGNORE_FREE_RELAY_CONNECTIONS` | Skip already-connected SSIDs during scan to avoid loops in mesh |
| `ALLOW_WIFI_CONFIG_MODIFICATION`, `ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY` | Restrict the WiFi config web form |
| `ALLOW_MQTT_CONFIG_MODIFICATION`, `ALLOW_OTA_CONFIG_MODIFICATION` | Same for MQTT / OTA forms |
| `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` | If DB checksum invalid at boot, reset to defaults instead of halting |
| `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` | Factory reset writes default structs back instead of zeroing |
| `ENABLE_LOG_ALL` / `_INFO` / `_WARNING` / `_ERROR` / `_SUCCESS` | Per-level log gates; if none are set, the logger interface is dropped entirely |

#### 4.3.3 Per-device limits

| Macro | Purpose | Typical values |
|---|---|---|
| `MAX_DIGITAL_GPIO_PINS` | Size of GPIO config table | 14 (uno), 9 (esp32), 9 (esp8266) |
| `MAX_ANALOG_GPIO_PINS` | Same, analog side | 5 (uno), 4 (esp32), 1 (esp8266) |
| `MAX_DB_TABLES` | Upper bound on registered tables | 5 (uno), 15 (esp*) |
| `MAX_SCHEDULABLE_TASKS` | Inline scheduler slot count | 25 (in `Common.h`) |
| `MAX_FACTORY_RESET_CALLBACKS` | Reset-hook count | `MAX_SCHEDULABLE_TASKS` |
| `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` | STA connect retry budget (s) | 1 |
| `WIFI_CONNECTIVITY_CHECK_DURATION` | Inter-check interval (ms) | 5000 |
| `INTERNET_CONNECTIVITY_CHECK_DURATION` | Internet recheck cadence | same as above |

### 4.4 The service-tier `*Config.h` pattern

Every per-service config file follows the same shape so the framework can treat them uniformly. Using [WifiConfig.h](src/config/WifiConfig.h) as the canonical example:

```cpp
#define WIFI_CONFIGS_BUF_SIZE 30
#define DEFAULT_SSID         USER          // 1. defaults
#define DEFAULT_PASSPHRASE   PASSPHRASE
const uint8_t DEFAULT_AP_LOCAL_IP[] = {192, 168, 0, 1};
...

#define ALLOW_WIFI_CONFIG_MODIFICATION    // 2. policy switches (optional)

typedef struct { ... } __status_wifi_t;   // 3. runtime-only status (not persisted)
extern __status_wifi_t __status_wifi;

struct wifi_configs {                     // 4. the persisted struct
  wifi_configs() { clear(); }             //    - default-constructible
  void clear() { ... }                    //    - zero/default initialisable
  char sta_ssid[WIFI_CONFIGS_BUF_SIZE];   //    - fixed-size members only (no pointers)
  ...
};

const int wifi_config_size = sizeof(wifi_configs) + 5;  // 5. on-NVM size (+5 = framing)
using wifi_config_table = wifi_configs;                 // 6. canonical table alias
```

The six numbered conventions are part of the contract — the database layer ([§6](#6-database-layer-pending)) and the web forms rely on every config struct providing them.

#### 4.4.1 Why `+5` in `*_config_size`

Each persisted table is wrapped on NVM with a small framing record (table id + checksum). The `+ 5` byte allowance is the framework's fixed overhead per table, summed at boot to compute total NVM usage and validated against `iDatabaseInterface::getMaxDBSize()`.

#### 4.4.2 Why structs use `memset` + `memcpy` instead of initializers

Configs are placed in NVM directly via raw byte copy. The structs are POD-shaped (no virtuals, no pointers, fixed-size arrays) and `clear()` runs both at construction and at factory-reset; this guarantees the same byte layout on every load, which is what makes raw-NVM serialisation safe.

### 4.5 Per-service config files — what's in each

| File | Always-on table(s) | Headline knobs |
|---|---|---|
| [Common.h](src/config/Common.h) | — | `USER`, `PASSPHRASE`, `MAX_SCHEDULABLE_TASKS`, time durations, HTTP request budget |
| [GlobalConfig.h](src/config/GlobalConfig.h) | `global_config_table` (version, year, firmware) | `CONFIG_START`, `CONFIG_VERSION`, `FIRMWARE_VERSION`, `RELEASE`, `LAUNCH_UNIX_TIME` |
| [WifiConfig.h](src/config/WifiConfig.h) | `wifi_config_table` (STA+AP creds, IPs) | `WIFI_CONFIGS_BUF_SIZE`, default IPs, modification policies |
| [ServerConfig.h](src/config/ServerConfig.h) | `login_credential_table` (web/CLI user) | `SERVER_SESSION_NAME`, `SERVER_COOKIE_MAX_AGE` (300 s) |
| [HttpConfig.h](src/config/HttpConfig.h) | — (transient `http_param_t`) | `HTTP_CLIENT_BUF_SIZE`, request size limits, `HTTPS_DEFAULT_PORT`, `HTTPS_HSTS_MAX_AGE_SECONDS` |
| [TlsConfig.h](src/config/TlsConfig.h) | — | `TLS_IBUF_SIZE` / `TLS_OBUF_SIZE` (per-session TLS record buffers, default 2 KB / 1 KB), `TLS_TASK_STACK_SIZE` (default 6500 B for the dedicated cooperative TLS task on esp8266), `TLS_TASK_POLL_MS`, plus default FS paths `TLS_DEFAULT_SERVER_CERT_PATH` (`/etc/http/server.crt`), `TLS_DEFAULT_SERVER_KEY_PATH`, `TLS_DEFAULT_CLIENT_CA_PATH`, `TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH` (`/etc/ssl/ca-bundle.crt`) |
| [OtaConfig.h](src/config/OtaConfig.h) | `ota_config_table` (host, port, version, freq) | OTA check cadence |
| [MqttConfig.h](src/config/MqttConfig.h) | `mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table` | Broker, LWT, publish/subscribe slots, `MQTT_CONFIG_TYPE` selector |
| [GpioConfig.h](src/config/GpioConfig.h) | `gpio_config_table` | Pin map, `GPIO_MODE` enum, event conditions/channels, graph dimensions |
| [EmailConfig.h](src/config/EmailConfig.h) | `email_config_table` | SMTP host/port/auth, default subject |
| [DeviceIotConfig.h](src/config/DeviceIotConfig.h) | `device_iot_config_table` | Config/OTP URLs, channel keys, sampling/keepalive bounds |
| [SerialConfig.h](src/config/SerialConfig.h) | — | `SERIAL_MODE`, baud, interface enum |
| [StorageConfig.h](src/config/StorageConfig.h) | — | Mount point, max path, FS limits |
| [SshConfig.h](src/config/SshConfig.h) | — | `SSHKeyAlgorithm`, key/file size caps |
| [NetworkConfig.h](src/config/NetworkConfig.h) | — | Network-wide timeouts |
| [EventConfig.h](src/config/EventConfig.h) | — | `event_name` enum, channel registry |

> **Pattern note:** the always-on tables (`global_config_table`, `login_credential_table` when auth is on, `wifi_config_table` when wifi is on) form the framework's minimum NVM footprint. Everything else is opt-in via the matching `ENABLE_*`.

### 4.6 Dependency rules (read before flipping a flag)

The most common foot-gun is enabling a service whose dependency is off. The rules:

- `ENABLE_HTTP_SERVER` ⇒ `ENABLE_WIFI_SERVICE` (web server must run somewhere) ⇒ `ENABLE_NETWORK_SERVICE`.
- `ENABLE_MQTT_SERVICE` / `ENABLE_OTA_SERVICE` / `ENABLE_EMAIL_SERVICE` / `ENABLE_DEVICE_IOT` ⇒ `ENABLE_NETWORK_SERVICE` (need TCP client) and *usually* `ENABLE_WIFI_SERVICE` (need a network).
- `ENABLE_SSH_SERVICE` ⇒ `ENABLE_NETWORK_SERVICE` **and** `ENABLE_STORAGE_SERVICE` (key persistence, SFTP).
- `ENABLE_TELNET_SERVICE` ⇒ `ENABLE_NETWORK_SERVICE` and benefits from `ENABLE_CMD_SERVICE` (no CLI = no terminal value).
- `ENABLE_AUTH_SERVICE` ⇒ `ENABLE_STORAGE_SERVICE` (credentials need to survive reboot) — though framed via the always-present DB.
- `ENABLE_DEVICE_IOT` ⇒ the **application** must implement `iDeviceIotInterface` (see [§3.3.5](#3-portable-interfaces)) and pass its instance to `__device_iot_service.initService(...)`.
- `ENABLE_CONTEXTUAL_EXECUTION` ⇒ the port supplies the full threading interface family ([§3.3.6](#3-portable-interfaces)) — both esp8266 and esp32 ship this today.
- `ENABLE_TLS_SERVICE` ⇒ implicitly turns on `ENABLE_CONTEXTUAL_EXECUTION` (TLS runs on its own cooperative task — see [TlsConfig.h](src/config/TlsConfig.h)). Cannot coexist with `ENABLE_NAPT` on esp8266 (both want too much heap). Auto-`#undef`ed on UNO along with storage and contextual execution.
- `ENABLE_HTTPS_SERVER` ⇒ `ENABLE_TLS_SERVICE` **and** `ENABLE_HTTP_SERVER` **and** `ENABLE_STORAGE_SERVICE` (cert/key are loaded from FS at `/etc/http/server.{crt,key}`). Optional `ENABLE_HTTPS_SERVER_MTLS` adds `/etc/http/client-ca.crt` mTLS verification.
- `ENABLE_TLS_CERT_GENERATION` ⇒ `ENABLE_TLS_SERVICE` **and** `DEVICE_ESP32` (mbedTLS issuer API). The `tls` CLI command is gated on this flag.
- `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` ⇒ `ENABLE_TLS_CERT_GENERATION`. Registers a listener on `EVENT_WIFI_STA_GOT_IP` that mints a self-signed cert for the device IP the first time it boots without one.
- Disabling **all** `ENABLE_LOG_*` removes `iLoggerInterface` from the build — your port may then omit its `LoggerInterface.{h,cpp}`.

The header guards in [devices/DeviceConfig.h](devices/DeviceConfig.h) enforce most of these by structurally wrapping the dependent flags inside `#ifdef ENABLE_NETWORK_SERVICE`. The rest you are responsible for.

### 4.7 Common build shapes

A few preset configurations contributors typically reach for:

| Goal | Flags to keep | Flags to drop |
|---|---|---|
| **Smallest possible** (uno-class) | `SERIAL`, `CMD`, `GPIO_BASIC_ONLY` | Everything else |
| **Offline gateway** | Add `STORAGE`, `AUTH` | Everything network |
| **Headless networked node** | Add `NETWORK`, `WIFI`, `MQTT`, `OTA`, `NTP` | `HTTP_SERVER`, `SSH`, `EMAIL` |
| **Full portal** | Default `DeviceConfig.h` (everything on) | — |
| **Diagnostics build** | Default + `ENABLE_LOG_ALL` | — |
| **Concurrent demo** (esp8266 or esp32) | Default + `ENABLE_CONTEXTUAL_EXECUTION` | — |
| **HTTPS portal** | Default + `ENABLE_TLS_SERVICE` + `ENABLE_HTTPS_SERVER` (esp32 also: `ENABLE_TLS_CERT_GENERATION`) | `ENABLE_NAPT` on esp8266 |
| **HTTPS + mTLS** | Above + `ENABLE_HTTPS_SERVER_MTLS` | — |

### 4.8 Changing defaults vs persisted values

A frequent point of confusion: editing `DEFAULT_SSID` in `WifiConfig.h` does **not** change the SSID a running device uses. The defaults are only consulted on:

1. First boot (NVM empty / checksum invalid).
2. Factory reset (if `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` is set).

To change a live value, edit it through the web portal, the CLI (`net connsta`, `iot sethost`, etc.), or directly via `DatabaseServiceProvider::set(...)` and persist the table. The defaults govern the **fallback** state, not the **current** state.

### 4.9 Anti-patterns

Avoid:

- Adding runtime branches on `ENABLE_*` macros — they are compile-time. Use `#ifdef` instead, since the unreached branch otherwise pulls symbols the link will fail on.
- Putting a `std::string` / `pdiutil::string` or any pointer in a `*_config_table` struct — NVM serialisation requires fixed-size, POD layout.
- Cross-including `*Config.h` files. If a value is shared, hoist it to `Common.h`.
- Hard-coding feature checks in services against device macros (`#if defined(DEVICE_ESP32)`) — gate on the *capability* flag (`ENABLE_WIFI_SERVICE`), not the *board*. The board may change; the capability is what the code actually needs.

---

## 5. Task Scheduler

The scheduler is the framework's pacemaker. Every periodic background activity — NTP refresh, MQTT reconnect, OTA poll, GPIO blink, web session expiry, watch-dog feed, log heartbeat — runs through it. There is one inline scheduler that every device gets for free, plus two optional contextual lanes (cooperative and preemptive) that a port may add by implementing the threading interfaces from [§3.3.6](#3-portable-interfaces).

Implementation: [src/utility/TaskScheduler.h](src/utility/TaskScheduler.h), [src/utility/TaskScheduler.cpp](src/utility/TaskScheduler.cpp). Global instance: `__task_scheduler`.

### 5.1 Three task modes

| Mode | Enum value | Where the work runs | Stack used | Needs port support? | Use when |
|---|---|---|---|---|---|
| `TASK_MODE_INLINE` | `0` (default) | Inside `__task_scheduler.run()` on the main loop's stack | Main `loop()` stack | No | The task is short and non-blocking — periodic polls, timeouts, blink ticks. **This is the default for nearly everything.** |
| `TASK_MODE_COOPERATIVE` | `1` | On a dedicated stack via `__i_cooperative_scheduler.run()`, advancing only when the task explicitly `yield()`s or `sleep()`s | Per-task stack you size | Yes — port must provide `iCooperativeScheduler` | The task wants to *appear* sequential (`while(1) { do; sleep(500); }`) but you accept that one misbehaving task can starve others. |
| `TASK_MODE_PREEMPTIVE` | `2` | On a dedicated stack interrupted by a timer ISR via `__i_preemptive_scheduler.yield()` | Per-task stack you size | Yes — port must provide `iPreemptiveScheduler`, `iContext`, tick ISR | The task may block or run long and you cannot trust it to yield. Higher overhead per switch. |
| `TASK_MODE_MAX` | `3` | — | — | — | Sentinel; do not use |

All three modes coexist in the same process. `PDIStack::serve()` ticks each lane in order ([src/PdiStack.cpp](src/PdiStack.cpp)):

```cpp
__task_scheduler.run();                 // inline lane
__i_dvc_ctrl.yield();
__i_dvc_ctrl.handleEvents();
#ifdef ENABLE_CONTEXTUAL_EXECUTION
__i_cooperative_scheduler.tick_from_loop();  // cooperative lane (yields one slice per serve())
__i_preemptive_scheduler.yield();       // preemptive lane (only yields; ISR-driven)
#endif
```

> `tick_from_loop()` exists on every `iExecutionScheduler` since the TLS work landed — for cooperative schedulers it forwards to `run()` (one slice from the main loop), and preemptive schedulers leave it as a no-op because they're driven by a hardware timer. The orchestrator now calls `tick_from_loop()` instead of `run()` so any port that needs a different main-loop entry point (timer-only, deferred-tick, etc.) can override just that hook.

### 5.2 Task policies (scheduling discipline within a mode)

Independent of mode, each task carries a `task_policy_t` that influences which **inline** task runs first when several are due at the same tick:

| Policy | Enum | Selection rule |
|---|---|---|
| `TASK_POLICY_FIFO` | `0` (default) | Strict by priority, with overdue tasks getting a small nudge (lateness boost capped at 50). |
| `TASK_POLICY_ROUNDROBIN` | `1` | Equal time slices — favours tasks that ran least recently (log-scaled). |
| `TASK_POLICY_DEADLINE` | `2` | Earliest deadline first — boost = `2 ×`. |
| `TASK_POLICY_FAIRSHARE` | `3` | Favours tasks that have consumed the **least** cumulative CPU time so far. Boost = `1.5 ×`. |

The scoring formula is in [TaskScheduler::computeScore](src/utility/TaskScheduler.cpp). All four share the same base term `priority × 100`, then add a policy-dependent boost capped at `50`. The practical implication: **priority always dominates policy** by a wide margin; policy is the tie-breaker.

> Policies apply to the inline lane. Contextual lanes have their own per-implementation scheduling.

### 5.3 The `task_t` record

Defined in [src/utility/DataTypeDef.h](src/utility/DataTypeDef.h).

| Field | Type | Meaning |
|---|---|---|
| `_task_id` | `int` | Unique id (1..`MAX_SCHEDULABLE_TASKS-1`), allocated by `get_unique_task_id` |
| `_task` | `CallBackVoidArgFn` (`pdiutil::function<void()>`) | The callback |
| `_duration` | `uint64_t` (ms) | Interval (for periodic) or timeout (for one-shot) |
| `_last_millis` | `uint64_t` (ms) | Timestamp of last run; advanced in catch-up steps of `_duration` |
| `_task_exec_millis` | `uint64_t` (ms) | Wall-clock time the last invocation took — used for fair-share scoring |
| `_task_priority` | `int` | Higher = runs first (default `DEFAULT_TASK_PRIORITY = 0`) |
| `_task_policy` | `task_policy_t` | FIFO/RR/Deadline/Fairshare, default FIFO |
| `_task_mode` | `task_mode_t` | INLINE/COOPERATIVE/PREEMPTIVE, default INLINE |
| `_max_attempts` | `int` | `-1` = unlimited; `0` = remove next sweep; positive count = N runs then expire |

The scheduler stores tasks in a `pdiutil::vector<task_t>` reserved to `MAX_SCHEDULABLE_TASKS` slots — there is no heap churn during steady-state operation.

### 5.4 Public API by use case

| Want to… | Call | Returns | Notes |
|---|---|---|---|
| Run something once after N ms | `setTimeout(fn, dur, now)` | task id | Sets `_max_attempts = 1`; task is auto-removed after firing |
| Run something every N ms | `setInterval(fn, dur, now)` | task id | `_max_attempts = -1` |
| Reschedule an existing timeout | `updateTimeout(id, fn, dur, now)` | id | Replaces the callback/duration in place |
| Reschedule an existing interval | `updateInterval(id, fn, dur, prio, last, maxAtt)` | id | Falls back to `register_task` if id not found |
| Register a task with custom priority / attempts | `register_task(fn, dur, prio, last, maxAtt)` | id | The lowest-level entry point |
| Cancel a one-shot | `clearTimeout(id)` | bool | Marks the task for removal next sweep (see §5.7) |
| Cancel a periodic | `clearInterval(id)` | bool | Same |
| Look up a task | `get_task(id)` | `task_t*` or `nullptr` | Useful to mutate `_task_policy` / `_task_mode` after registration |
| Force a re-sort mid-sweep | `rebaseAndRestartPrioTasks()` | void | Breaks out of the current `handle_tasks` loop on the next iteration |
| Print active tasks to terminal | `printTasksToTerminal(t)` | void | Backs the `scht` CLI command |
| Promote a task to a contextual lane | `scheduleUnderExecSched(sched, id, mode, stackdepth)` | int (negative on failure — typically `-1`/`-99` if the port doesn't supply the scheduler or the task couldn't be queued) | Requires `ENABLE_CONTEXTUAL_EXECUTION` |

`register_task` returns `-1` if the slot table is full. Callers must check the return value before assuming the task was accepted.

### 5.5 How a tick is processed

`__task_scheduler.run()` calls `handle_tasks()`, which on every tick:

1. Reads `now = m_util->millis_now()`.
2. Builds an array of indices and sorts it via `getSortedTaskList` using `computeScore` plus a 3 ms tolerance window on due-times. **The vector itself is not reordered** — only an indirection table — so task ids stay stable.
3. Iterates the sorted indices, skipping any task whose `_task_mode != TASK_MODE_INLINE` when contextual execution is compiled in.
4. For each due task (`now - _last_millis ≥ _duration`), invokes the callback, advances `_last_millis` by whole multiples of `_duration` (catch-up, capped at 3 rounds to avoid runaway after a long stall), decrements `_max_attempts` if finite, and records `_task_exec_millis`.
5. Calls `m_util->yield()` after each task so the platform can service WiFi / interrupts.
6. If `m_rebase_start_priotask` was set (either by another task calling `rebaseAndRestartPrioTasks` or implicitly at the top of `handle_tasks`), breaks out of the loop after running **one** task — the next tick will re-sort. This guarantees a newly-registered high-priority task is picked up on the very next tick.
7. Calls `remove_expired_tasks` to drop everything with `_max_attempts == 0`.

The "run-one-then-break" pattern (point 6) is the key reason the scheduler keeps low jitter without preemption: each `serve()` iteration runs at most one inline task before yielding back to the platform, the web server, and the contextual lanes.

### 5.6 Choosing a mode — decision tree

```
Does your task do real work for more than a few ms,
OR call a blocking SDK API (TLS handshake, large flash write)?
│
├─ No  → TASK_MODE_INLINE  (the default; no port effort)
│
└─ Yes → Is the work sequential / stateful enough that you'd write
         it as `while(1){ step(); sleep(N); }` if you could?
         │
         ├─ Yes → TASK_MODE_COOPERATIVE
         │        (you control yields; cheap context switches;
         │         starvation possible — peer tasks rely on you)
         │
         └─ No, it might block unpredictably → TASK_MODE_PREEMPTIVE
                  (timer ISR forces switches; safe against badly
                   behaved code; higher per-switch overhead;
                   needs `iContext` + tick ISR from the port)
```

If your port does not provide the contextual interfaces, **the only honest choice is INLINE plus splitting the work into a state machine**. Calling `scheduleUnderExecSched` with a missing scheduler is a no-op.

### 5.7 Threading and lifecycle gotchas

A few sharp edges worth knowing before you use the API in anger:

- **`remove_task` does not erase immediately.** It marks the task by zeroing `_task`, dropping priority, and setting `_max_attempts = 0`; the actual `vector::erase` happens in `remove_expired_tasks` at the *end* of `handle_tasks`. This is intentional — erasing during iteration was a bug source ([TaskScheduler.cpp](src/utility/TaskScheduler.cpp)). Don't dereference an id immediately after `clearInterval`.
- **Tasks are stored by value.** Lambdas that capture a `this` pointer or large state work fine, but be mindful that captures live until the task is removed *and* the next sweep runs.
- **The scheduler is not re-entrant.** Don't call `__task_scheduler.run()` from inside a task. Use the contextual lanes if you need nested execution.
- **`_last_millis = 0` means "first run".** On the first dispatch the scheduler stamps `_last_millis = now` instead of doing catch-up, so a freshly registered interval will fire after one full `_duration` from the moment it is first considered.
- **Catch-up is capped at 3 rounds.** A scheduler that has been frozen for >3× the task duration will resume fresh rather than firing all the back-runs. Long blocks (OTA, deep sleep) should re-register their tasks if they need exact-count semantics.
- **`MAX_SCHEDULABLE_TASKS` defaults to 25** ([src/config/Common.h](src/config/Common.h)). `PDIStack::PDIStack()` configures the scheduler with that limit. If you raise it, every service's worst-case footprint grows along with `MAX_FACTORY_RESET_CALLBACKS`.
- **No clock skew tolerance beyond 3 ms.** If your `iUtilityInterface::millis_now()` jitters more than that, you will see tasks reshuffle order between runs. The 3 ms window in `getSortedTaskList` is per-comparison, not per-tick.

### 5.8 Contextual scheduling — extra API

When `ENABLE_CONTEXTUAL_EXECUTION` is on, the inline scheduler **registers** the task (you get an id) and then a second call **promotes** it to a contextual lane:

```cpp
int tid = __task_scheduler.register_task([&](){
    uint32_t i = 0;
    while (1) {
        // long-running body, allowed to sleep
        __i_dvc_ctrl.getTerminal()->writeln((uint32_t)i++);
        __i_cooperative_scheduler.sleep(500);  // yield to peers
    }
});
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler,
    tid,
    TASK_MODE_COOPERATIVE,
    1 * 1024                  // per-task stack bytes
);
```

Stack sizing is your responsibility — there is no automatic measurement unless the port supports `iUtilityInterface::can_measure_stack()`. A safe starting point on esp8266 is 1 KiB cooperative / 2 KiB preemptive; bump it if your task uses `pdiutil::string`, JSON parsing, or TLS.

Cross-lane primitives:

- `iCooperativeScheduler::sleep(ms)` / `::yield()` — must be called from inside the cooperative task; the inline lane's `sleep()` is a no-op by design ([TaskScheduler.cpp](src/utility/TaskScheduler.cpp)).
- `iPreemptiveScheduler::yield()` — voluntary yield from a preemptive task; the tick ISR will preempt it anyway.
- `iMutex` / `iConditionVar` — required whenever data crosses lanes. The inline lane does not need them.

### 5.9 Inspecting and tuning at runtime

- **`scht` CLI command** prints every registered task with id, priority, policy, interval, last-run timestamp, last execution duration, and remaining attempts. Source: [printTasksToTerminal](src/utility/TaskScheduler.cpp).
- **`watch` CLI command** uses the scheduler internally to re-run another command on a fixed interval (`watch c=net ip; i=3000; n=10`).
- **`srvc` CLI** lists each service's currently-held task id (`m_service_routine_task_id` on the `ServiceProvider` base) so you can correlate scheduler rows back to services.

### 5.10 Authoring patterns

A few patterns worth copying when you write a new service:

**Self-rescheduling state machine.** Don't loop inside a task; advance one step and return:

```cpp
__task_scheduler.setInterval([&](){
    switch (m_state) {
        case S_IDLE:        m_state = startConnect(); break;
        case S_CONNECTING:  m_state = pollConnect();  break;
        case S_SENDING:     m_state = pushNextChunk(); break;
        case S_DONE:        m_state = S_IDLE;          break;
    }
}, /*ms*/ 50, m_util->millis_now());
```

**One-shot with rescheduling on failure.** Use `setTimeout` and re-register from within on retry:

```cpp
void scheduleProbe(uint32_t delay_ms) {
    __task_scheduler.setTimeout([this, delay_ms](){
        if (!probeOnce()) scheduleProbe(delay_ms * 2); // exponential backoff
    }, delay_ms, m_util->millis_now());
}
```

**Bound a service's footprint.** Stash the id in `ServiceProvider::m_service_routine_task_id` so the base `stopService` can cancel it for you ([ServiceProvider.h](src/service_provider/ServiceProvider.h)).

**Promote to a lane only when needed.** Register inline first; promote to cooperative only after profiling shows the task is the bottleneck. Premature promotion costs RAM you cannot give back.

---

## 6. Database Layer

The database layer persists framework configuration to non-volatile memory. It is **not** a general-purpose KV store, a relational DB, or a filesystem; it is a fixed-address, fixed-size, type-safe table store, sized to fit in a few KB of EEPROM-emulated flash. Every persisted struct from [§4](#4-configuration-system) reaches NVM through it.

It is split across the same three-tier shape the rest of the framework uses:

| Tier | Component | Path | Role |
|---|---|---|---|
| Port | `iDatabaseInterface` + concrete `DatabaseInterface` | [src/interface/pdi/iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h), `devices/<board>/DatabaseInterface.*` | Raw NVM read/write/checksum for one block at an address |
| Engine | `Database` + `DatabaseTable<addr, T>` | [src/utility/Database.{h,cpp}](src/utility/Database.h), [src/database/core/DatabaseTable.h](src/database/core/DatabaseTable.h) | Table registry, address-overlap checks, typed `get/set/clear` |
| Service | `DatabaseServiceProvider` | [src/service_provider/database/DatabaseServiceProvider.h](src/service_provider/database/DatabaseServiceProvider.h) | Public `get_*_table` / `set_*_table` APIs, factory-reset wiring |

The per-table classes themselves are **autogenerated** from a JSON schema by [scripts/CreateDBSourceFromJson.py](scripts/CreateDBSourceFromJson.py) — they live in [src/database/tables/](src/database/tables/) and should never be hand-edited.

### 6.1 Mental model

NVM is treated as a flat byte address space owned by the port:

```
NVM (size = __i_db.getMaxDBSize())
0          5                                                                  end
├──────────┼──────────┬──────────┬──────────┬──────────┬──────────┬──── ... ──┤
│ reserved │ Global   │ Login    │ WiFi     │ OTA      │ Gpio     │           │
│  prefix  │  @5      │  @50     │  @150    │  @300    │  @500    │  unused   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──── ... ──┘
                ↑                                                            ↑
        CONFIG_START                                          getMaxDBSize() - footer
```

Each table occupies `[address, address + sizeof(T) + 5)` — `+5` is the per-table framing the port writes (table id byte + 4-byte CRC/checksum). Addresses are **hard-coded** per table in the autogenerated header; the engine's only address-time check is "does this table overlap or run past the limit". There is no allocator, no relocation, no defragmentation.

### 6.2 The port — `iDatabaseInterface`

The port exposes four virtual methods plus three implementation-required template methods (declared in comments, [iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h)):

| Member | Contract |
|---|---|
| `beginConfigs(size)` | Open / mount the NVM region of `size` bytes. Called once from `DatabaseServiceProvider::initService` |
| `cleanAllConfigs()` | Erase the entire region |
| `isValidConfigs()` | Return `true` if the framing/checksum is intact across all tables — drives auto factory reset |
| `getMaxDBSize()` | Total NVM budget, in bytes |
| *template* `saveConfig<T>(addr, *obj)` | Write `sizeof(T)` bytes at `addr` plus framing |
| *template* `loadConfig<T>(addr, *obj)` | Read `sizeof(T)` bytes into `*obj` |
| *template* `clearConfig<T>(addr)` | Reset the block at `addr` to its default-constructed `T` |

The three templated methods are declared in the abstract base only as commented contracts because virtual templates aren't legal in C++. Each port's concrete `DatabaseInterface` class **must** define them — typically as inline templates in `DatabaseInterface.h` so callers (like `DatabaseTable`) instantiate them at use sites.

Singleton: `extern DatabaseInterface __i_db;`

### 6.3 The engine — `Database` and `DatabaseTable<addr, T>`

The engine is purely an in-RAM registry of `struct_tables { addr, size, instance* }` records — it owns no bytes itself.

#### `DatabaseTableAbstractLayer`
Every table inherits this. Its constructor self-registers the table into a static array, bounded by `MAX_TABLES = MAX_DB_TABLES` ([Database.h](src/utility/Database.h)) — so tables are visible to the engine *before* `main()` runs. No dynamic dispatch needed for registration.

#### `DatabaseTable<uint16_t addr, class Table>`
The CRTP-style template each generated class inherits from:

```cpp
class WiFiTable : public DatabaseTable<WIFI_CONFIG_TABLE_ADDRESS, wifi_config_table> {};
extern WiFiTable __wifi_table;
```

This gives every table four members for free:

| Method | Effect |
|---|---|
| `boot()` | Adds the `(addr, size, this)` record to `__database.m_database_tables` via `register_table`; called once by `Database::init_database` |
| `get(T*)` | `__i_db.loadConfig<T>(addr, T*)` |
| `set(T*)` | `__i_db.saveConfig<T>(addr, T*)` |
| `clear()` | `__i_db.clearConfig<T>(addr)` — resets the slot to the `T` default constructor |

#### `Database` (singleton `__database`)
Holds the registry vector and enforces three rules during `register_table`:

1. The new table's `address` must be **greater than** `last_table.address + last_table.size + 2` (no overlap, with a 2-byte gap).
2. The new table's tail (`address + size + 2`) must be **less than** `m_max_db_size`.
3. Total registered tables must be `≤ MAX_DB_TABLES`.

Violation: `register_table` returns `false` and the table is silently skipped. Callers must verify, but the typical flow (auto-registration at boot) just leaves the table unavailable — a hint to bump `MAX_DB_TABLES` in `DeviceConfig.h` or compact the address map.

`clear_all()` iterates every registered table and calls its `clear()` — used by the factory-reset path.

### 6.4 The service — `DatabaseServiceProvider`

`DatabaseServiceProvider` ([source](src/service_provider/database/DatabaseServiceProvider.h)) is what the rest of the framework actually calls. It does three things:

1. **Defines the global table singletons.** `GlobalTable __global_table;`, `WiFiTable __wifi_table;`, … each guarded by the matching `ENABLE_*` flag so disabled features cost zero ([DatabaseServiceProvider.cpp](src/service_provider/database/DatabaseServiceProvider.cpp)).
2. **Owns the boot sequence** in `initService` ([](src/service_provider/database/DatabaseServiceProvider.cpp)):
   ```cpp
   __i_db.beginConfigs(__i_db.getMaxDBSize());     // open NVM
   __database.init_database(__i_db.getMaxDBSize());// reserves vector, calls boot() on every table
   if (AUTO_FACTORY_RESET_ON_INVALID_CONFIGS) {
       __task_scheduler.setInterval([&]{
           if (!__i_db.isValidConfigs()) {
               clear_default_tables();
               __factory_reset.factory_reset();
           }
       }, 5000, now);
   }
   if (CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET) {
       __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&]{ clear_default_tables(); });
   }
   ```
3. **Exposes typed `get_*_table` / `set_*_table` pairs**, one per persisted struct. These are thin pass-throughs to `__<name>_table.get/set` and are the **only** public surface other services should touch.

> Services should never call `__i_db` directly or hold a raw `__<name>_table` pointer. Going through `__database_service` keeps the persistence boundary one-way: services depend on the service, not on the port or codegen layout.

### 6.5 The autogen pipeline

Per-board tables come from a per-device schema, not the framework root:

```
devices/<board>/config/DBTableSchema.json     ← edit this to add/move a table
        │
        │  scripts/DeviceSetup.py -d <board>
        │      → invokes scripts/CreateDBSourceFromJson.py
        │           → invokes scripts/JsonToCpp.py
        ▼
src/database/tables/<TableName>.h             ← generated; do not edit
```

The schema's `defItems` list each table. Per entry:

| Field | Meaning |
|---|---|
| `defItemName` | C++ class name (e.g. `WiFiTable`) |
| `defItemDesc` | Doc-comment text |
| `defItemArg` | The `using *_table = struct ...` alias from `*Config.h` (e.g. `wifi_config_table`) |
| `defItemAddressKey` | Macro that becomes the `#define <name>` in the generated header (e.g. `WIFI_CONFIG_TABLE_ADDRESS`) |
| `defItemAddressValue` | Numeric (or symbolic) byte offset in NVM |
| `defItemExtVar` | The `extern <ClassName>` global the framework links against (e.g. `__wifi_table`) |

The full schema template — class template, parent (`DatabaseTable<addr, type>`), included headers, post-declaration `extern` — lives at the top of [devices/esp32/config/DBTableSchema.json](devices/esp32/config/DBTableSchema.json). A generated table is always a `final-shape, no-member` subclass of `DatabaseTable<…>` — all logic lives in the template.

#### 6.5.1 Default address map (esp* schema)

| Table | Address | Backing struct | Service flag |
|---|---|---|---|
| Global | `CONFIG_START` (5) | `global_config_table` | always |
| Login | 50 | `login_credential_table` | `AUTH` ∥ `HTTP_SERVER` |
| WiFi | 150 | `wifi_config_table` | `WIFI` |
| OTA | 300 | `ota_config_table` | `OTA` |
| GPIO | 500 | `gpio_config_table` | `GPIO` |
| MQTT General | 700 | `mqtt_general_config_table` | `MQTT` |
| MQTT LWT | 1500 | `mqtt_lwt_config_table` | `MQTT` |
| MQTT PubSub | 1700 | `mqtt_pubsub_config_table` | `MQTT` |
| Email | 2000 | `email_config_table` | `EMAIL` |
| Device IoT | 2600 | `device_iot_config_table` | `DEVICE_IOT` |

The gaps between addresses are deliberate growth headroom for the backing struct — if you add a field to `wifi_configs`, the WiFi table can grow up to `300 - 150 - 5 = 145` bytes before colliding with OTA. Going beyond requires moving OTA (and migrating).

### 6.6 Boot sequence (end-to-end)

```
static-init time
    each `<Table>Table __<name>_table;` global is constructed
        DatabaseTableAbstractLayer ctor pushes `this` into m_instances[]

setup() → PdiStack.initialize() → __database_service.initService()
    __i_db.beginConfigs(maxSize)              // port mounts NVM
    __database.init_database(maxSize)
        m_database_tables.reserve(MAX_TABLES)
        for each pre-registered instance:
            instance->boot()
                register_table({addr, sizeof(T), this})
                    if (no overlap && fits in maxSize) push_back
    if AUTO_FACTORY_RESET_ON_INVALID_CONFIGS:
        every 5s → if !__i_db.isValidConfigs() → clear_all + __factory_reset
    if CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET:
        listen for EVENT_FACTORY_RESET → clear_all
```

After this, any service can call `__database_service.get_wifi_config_table(&cfg)` and trust the contents.

### 6.7 Read / write semantics

- **Reads are full-struct.** There is no per-field read. Always allocate the full `T` on the stack, read into it, mutate, write back.
- **Writes are immediate.** `set_*_table` returns after the port's `saveConfig` returns — there is no journal, no commit phase. On flash-backed ports this is a flash write per call; **batch your `set` calls** if you are mutating several fields in a row.
- **Endianness and packing.** Structs are written raw via `memcpy`. The framework assumes the same toolchain reads the same bytes back, so a port that targets a different ABI cannot share an NVM image with another port. This is a tradeoff for zero serialisation overhead.
- **Concurrency.** None. The engine is single-threaded; if you use contextual execution, guard `get`/`set` calls with an `iMutex` if more than one lane touches the same table.

### 6.8 Factory reset

Two configurable behaviours:

| Macro | Effect |
|---|---|
| `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` | Every 5 s, check `__i_db.isValidConfigs()`. If false, `clear_all` + invoke `__factory_reset`. Recovers from corrupted flash without bricking the device |
| `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` | On `EVENT_FACTORY_RESET` (fired by `FactoryResetServiceProvider` after a 6-7s flash-button hold), clear all tables to default. Without this, `factory_reset` only triggers a reboot |

Other services can hook the same event ([EventConfig.h](src/config/EventConfig.h)) to perform their own cleanup (delete IoT cache, clear MQTT retained, etc.).

### 6.9 Adding a new table

To persist a new struct (say `metrics_config` for a new metrics service):

1. **Define the persisted struct** in a new or existing `src/config/<service>Config.h`, following the [§4.4](#4-configuration-system) contract — POD layout, default ctor calls `clear()`, fixed-size members only:
   ```cpp
   struct metrics_configs { ... char host[40]; uint16_t port; ... };
   const int metrics_config_size = sizeof(metrics_configs) + 5;
   using metrics_config_table = metrics_configs;
   ```
2. **Pick a free address** in the per-device schema. Use the gap calculator: `prev_addr + prev_size + 5 < your_addr < next_addr`. For esp* the schema currently runs to 2600 + `sizeof(device_iot_configs) + 5`, so anything beyond that is fair game.
3. **Add a `defItems` entry** to every `devices/<board>/config/DBTableSchema.json` that should carry the table:
   ```json
   {
     "defItemName": "MetricsTable",
     "defItemDesc": "Metrics table handles metrics service configs",
     "defItemArg": "metrics_config_table",
     "defItemAddressKey": "METRICS_CONFIG_TABLE_ADDRESS",
     "defItemAddressValue": "3000",
     "defItemExtVar": "__metrics_table"
   }
   ```
4. **Regenerate**: `python3 scripts/DeviceSetup.py -d <board>` — this rewrites `src/database/tables/MetricsTable.h`.
5. **Add the include + global + accessors** in `DatabaseServiceProvider.{h,cpp}` under the matching `ENABLE_METRICS_SERVICE` guard, mirroring `WiFiTable` / `OtaTable` / etc.
6. **If you exceeded `MAX_DB_TABLES`**, raise it in `devices/DeviceConfig.h` (currently 15 for esp*, 5 for uno).

### 6.10 Gotchas

- **Adding a field to a struct rewrites the on-flash layout.** Existing devices will see `isValidConfigs() == false` on next boot and (if `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` is on) get reset to defaults. There is no schema migration. Bump `CONFIG_VERSION` to make this intentional.
- **Address collisions are not flagged loudly.** `Database::register_table` returns `false` and the table is silently absent. If `get_wifi_config_table` is returning defaults after a schema edit, check whether your address overlaps with a neighbour.
- **`get_last_table` walks linearly.** Registration is O(n²) in table count — fine for 15, painful if you push past 50.
- **`Database::register_table` only compares against the *last* table.** Inserting a table out of address order (i.e. with an address lower than an already-registered one) bypasses the overlap check. Always keep `defItemAddressValue` strictly increasing in schema order, or define them in ascending order in `DatabaseServiceProvider.cpp`.
- **The port's templates must be visible at every call site.** Define `saveConfig`/`loadConfig`/`clearConfig` inline in `DatabaseInterface.h`, not in `.cpp`, or the link will fail for unused types.
- **The codegen blows away the output directory.** `CreateDBSourceFromJson.py` does `cleanpath(outpath)` before writing — anything else in `src/database/tables/` is deleted. Don't park unrelated files there.

---

## 7. Service Providers

A *service provider* is the framework's unit of feature. Each one lives under [src/service_provider/](src/service_provider/), derives from [`ServiceProvider`](src/service_provider/ServiceProvider.h) ([§1.3](#1-architecture-overview)), and is gated by exactly one `ENABLE_*` flag ([§4.3](#4-configuration-system)). This section is the per-service reference: what it does, what it depends on, what its public API looks like, and how it surfaces in the CLI and web portal.

### 7.1 Common shape

Every provider follows the same skeleton:

```cpp
class XServiceProvider : public ServiceProvider {
public:
    XServiceProvider() : ServiceProvider(SERVICE_X, RODT_ATTR("X")) {}
    bool initService(void* arg = nullptr) override;     // wired from PDIStack::initialize
    bool stopService()                  override;       // optional
    void printConfigToTerminal(iTerminalInterface*) override;   // backs `srvc s=N,q=1`
    void printStatusToTerminal(iTerminalInterface*) override;   // backs `srvc s=N,q=2`
};
extern XServiceProvider __x_service;
```

Conventions that hold across all providers:

- The global instance is `__<name>_service` (matches [§1.7](#1-architecture-overview) naming).
- The constructor passes a `service_t` enum value and a `RODT_ATTR` flash-string name; the base class self-registers it into `m_services[]` so the `srvc` CLI can enumerate it generically.
- Long-running work is driven by `__task_scheduler` (`setInterval` for periodic, `setTimeout` for one-shots / state-machine steps) and the returned task id is stored in `m_service_routine_task_id` so the base `stopService` can clean it up.
- Cross-service triggers flow through `__utl_event` events ([EventConfig.h](src/config/EventConfig.h)), not direct calls — this keeps the dependency graph one-way through the orchestrator.
- Persisted config lives in NVM through `__database_service.get_<name>_config_table` / `set_<name>_config_table` ([§6.4](#6-database-layer)); services never touch `__i_db` directly.

### 7.2 Service reference

The order below matches the orchestrator's init order in [PdiStack.cpp](src/PdiStack.cpp).

#### 7.2.1 `DatabaseServiceProvider` — `__database_service`

| Flag | Always on (boot dependency) |
|---|---|
| Source | [database/DatabaseServiceProvider.{h,cpp}](src/service_provider/database/DatabaseServiceProvider.h) |
| Depends on | `iDatabaseInterface`, `__factory_reset`, `__utl_event` |
| Init does | `__i_db.beginConfigs(max)` → `__database.init_database(max)` → boots every registered `<Name>Table` → schedules 5 s `isValidConfigs` check → listens for `EVENT_FACTORY_RESET` |
| Public API | `get_*_table(T*)` / `set_*_table(T*)` for every persisted config struct ([§6.4](#6-database-layer)) |
| CLI surface | None directly; every other service's `srvc` config is fetched through it |

Fully documented in [§6. Database Layer](#6-database-layer).

#### 7.2.2 `DeviceFactoryReset` — `__factory_reset`

| Flag | Always on |
|---|---|
| Source | [device/FactoryResetServiceProvider.{h,cpp}](src/service_provider/device/FactoryResetServiceProvider.h) |
| Depends on | `iDeviceControlInterface::isDeviceFactoryRequested`, `__utl_event` |
| Init does | Schedules `check_device_factory_request()` periodically to poll the flash-button (6-7 s hold) |
| Public API | `factory_reset()` — fires `EVENT_FACTORY_RESET` then restarts the device; `check_device_factory_request()` — manual poll |
| CLI surface | Triggered by `reboot` after factory action; web portal has a "Reset Factory" form |
| Event contract | Other services listen for `EVENT_FACTORY_RESET` to drop their own caches before the reboot |

#### 7.2.3 `SerialServiceProvider` — `__serial_service`

| Flag | `ENABLE_SERIAL_SERVICE` |
|---|---|
| Source | [transport/SerialServiceProvider.{h,cpp}](src/service_provider/transport/SerialServiceProvider.h) |
| Depends on | `iSerialInterface` |
| Init does | Opens the serial port at the configured baud, hooks `processSerial` into `iDeviceControlInterface::handleEvents` |
| Public API | `processSerial(serial_event_t*)`, `appendSerialJsonPayload(...)`, `applySerialJsonPayload(...)`, `isAllowedSerialPort(...)` — used by Device-IoT for sensor read/write across serial |
| CLI surface | Provides the **default terminal** for the CLI ([§10](#10-command-line--terminal-pending)); `srvc` prints baud and active port |

#### 7.2.4 `WiFiServiceProvider` — `__wifi_service`

| Flag | `ENABLE_WIFI_SERVICE` |
|---|---|
| Source | [network/WiFiServiceProvider.{h,cpp}](src/service_provider/network/WiFiServiceProvider.h) |
| Depends on | `iWiFiInterface` (passed via `initService(&__i_wifi)`), `__database_service` (wifi table), optionally `iPingInterface` |
| Init does | Configures AP from `wifi_config_table`, kicks off STA scan, registers periodic connectivity check (`WIFI_CONNECTIVITY_CHECK_DURATION = 5000 ms`), schedules NAPT-enable one-shot after STA up |
| Public API | `configure_wifi_access_point(*)`, `configure_wifi_station(*)`, `scan_aps_and_configure_wifi_station_async(n)`, `reconfigure_wifi_access_point()`, `handleInternetConnectivity()`, `handleWiFiConnectivity()` |
| Config knobs | `IGNORE_FREE_RELAY_CONNECTIONS`, `ENABLE_DYNAMIC_SUBNETTING`, `ENABLE_NAPT`, `ENABLE_INTERNET_BASED_CONNECTIONS` |
| CLI surface | `net ip`, `net scansta`, `net connsta,<ssid>,<pass>` |
| Web surface | "WiFi" section (STA + AP forms, scan, current status), gated by `ALLOW_WIFI_CONFIG_MODIFICATION` |
| Event contract | Emits `EVENT_WIFI_STA_CONNECTED` / `EVENT_WIFI_STA_GOT_IP` / `EVENT_WIFI_STA_DISCONNECTED` / `EVENT_WIFI_AP_STA(DIS)CONNECTED` / `EVENT_WIFI_INTERNET_UP` / `EVENT_WIFI_INTERNET_DOWN`; consumers (MQTT, OTA, Email, IoT, TLS cert provisioner) react |

#### 7.2.5 `OtaServiceProvider` — `__ota_service`

| Flag | `ENABLE_OTA_SERVICE` |
|---|---|
| Source | [device/OtaServiceProvider.{h,cpp}](src/service_provider/device/OtaServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `iUpgradeInterface` (via `__i_dvc_ctrl`), `iHttpClientHelper`, OTA config table |
| Init does | `setInterval(handleOta, ota_freq_ms)` from `ota_config_table::ota_request_freq` |
| Public API | `handle()` → `http_ota_status`; `handleOta()` — periodic driver |
| Wire protocol | `GET /ota?mac_id=<mac>&version=<current>` → JSON `{ "latest": <ver> }`; if newer, `GET /bin/<mac>/<ver>.bin` and apply via `iUpgradeInterface::Upgrade` |
| Config knobs | `ALLOW_OTA_CONFIG_MODIFICATION`; `ota_config_table` (host, port, version, frequency) |
| CLI surface | `srvc` config print |
| Web surface | "OTA" section |

#### 7.2.6 `GpioServiceProvider` — `__gpio_service`

| Flag | `ENABLE_GPIO_SERVICE` (`ENABLE_GPIO_BASIC_ONLY` trims to digital-only for uno) |
|---|---|
| Source | [device/GpioServiceProvider.{h,cpp}](src/service_provider/device/GpioServiceProvider.h) |
| Depends on | `iGpioInterface` (folded into `__i_dvc_ctrl`), `gpio_config_table`, optionally `iTcpClientInterface` (POST sensor data), `__email_service` (event alerts) |
| Init does | Loads `gpio_config_table`, schedules `handleGpioOperations` (mode/value tick) and `handleGpioModes` (table refresh every `GPIO_TABLE_UPDATE_DURATION = 300000 ms`) |
| Public API | `handleGpioOperations()`, `handleGpioModes(type)`, `handleGpioHttpRequest(isEvent)`, `handleGpioEventOverEmail()`, `applyGpioJsonPayload(...)`, `applyGpioEventJsonPayload(...)`, `appendGpioJsonPayload(...)`, `setDeviceId(...)`, `setHttpHost(...)`, `enable_update_gpio_table_from_copy()` |
| Modes | `OFF`, `DIGITAL_WRITE`, `DIGITAL_READ`, `DIGITAL_BLINK`, `ANALOG_WRITE`, `ANALOG_READ` ([GpioConfig.h](src/config/GpioConfig.h)) |
| CLI surface | `gpio p=<pin>,m=<mode>,v=<value>` |
| Web surface | "GPIO" section + "GPIO Events" submenu (per-pin event conditions over `EMAIL` / `HTTP` channels) |

#### 7.2.7 `MqttServiceProvider` — `__mqtt_service`

| Flag | `ENABLE_MQTT_SERVICE` |
|---|---|
| Source | [transport/MqttServiceProvider.{h,cpp}](src/service_provider/transport/MqttServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table` |
| Init does | Opens broker connection (with LWT), subscribes from `pubsub` table, schedules publish driver |
| Public API | `handleMqttPublish(sync=false)`, `handleMqttSubScribe()`, `handleMqttConfigChange(type=MQTT_GENERAL_CONFIG)`, `setMqttPublishDataCallback(cb)`, `setMqttSubscribeDataCallback(cb)`, `stop()` |
| Callback types | `MqttPublishDataCallback`, `MqttSubscribeDataCallback` — applications register these to inject/receive payloads |
| Config knobs | `ENABLE_MQTT_DEFAULT_PAYLOAD`, `ALLOW_MQTT_CONFIG_MODIFICATION` |
| CLI surface | `srvc` config |
| Web surface | "MQTT" section with General / LWT / PubSub sub-forms |

#### 7.2.8 `EmailServiceProvider` — `__email_service`

| Flag | `ENABLE_EMAIL_SERVICE` |
|---|---|
| Source | [email/EmailServiceProvider.{h,cpp}](src/service_provider/email/EmailServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `email_config_table`, SMTP transport ([src/transports/smtp/](src/transports/smtp/)) |
| Init does | Loads `email_config_table`; schedules `handleEmail` if periodic mail enabled |
| Public API | `sendMail(pdiutil::string&)`, `sendMail(char*)`, `sendMail(const char*)`, `handleEmail()` |
| Config knobs | `DEFAULT_MAIL_HOST`, `DEFAULT_MAIL_PORT` (2525), `DEFAULT_MAIL_FROM/TO/SUBJECT`, `DEFAULT_MAIL_FREQUENCY` (300 s) |
| Used by | `GpioServiceProvider` for event-over-email |
| Web surface | "Email" section with credential form + "Test" button |

#### 7.2.9 `DeviceIotServiceProvider` — `__device_iot_service`

| Flag | `ENABLE_DEVICE_IOT` |
|---|---|
| Source | [iot/DeviceIotServiceProvider.{h,cpp}](src/service_provider/iot/DeviceIotServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `iDeviceIotInterface` (**implemented by the application**, see [§3.3.5](#3-portable-interfaces)), `__mqtt_service`, `__database_service` (iot table) |
| Init does | Bootstraps registration via `DEVICE_IOT_OTP_REQ_URL` → `DEVICE_IOT_CONFIG_REQ_URL`, then `configureMQTT()` against the returned channel, then schedules `handleSensorData` at the configured `sample rate` |
| Public API | `initDeviceIotSensor(iDeviceIotInterface*)` — application calls this to register the data hook; `handleRegistrationOtpRequest`, `handleDeviceIotConfigRequest`, `handleConnectivityCheck`, `configureMQTT`, `handleServerConfigurableParameters`, `beginSensorData`, `handleSensorData` |
| Application hook | The user's sketch must implement `iDeviceIotInterface::{ init, sampleHook, dataHook(payload), resetSampleHook }` |
| Config keys (server-controlled) | `channelhost`, `channelport`, `channelread`, `channelwrite`, `token`, `keepalive`, `samplerate`, `datarate`, `reconfig`, … (all in [DeviceIotConfig.h](src/config/DeviceIotConfig.h)) |
| CLI surface | `iot setid,<duid>`, `iot getid`, `iot sethost,<host>`, `iot gethost` |
| Note | Currently uses its own MQTT config independent of `__mqtt_service`'s tables — see [README §Device Iot (beta)](#device-iot-beta) |

#### 7.2.10 `AuthServiceProvider` — `__auth_service`

| Flag | `ENABLE_AUTH_SERVICE` |
|---|---|
| Source | [auth/AuthServiceProvider.{h,cpp}](src/service_provider/auth/AuthServiceProvider.h) |
| Depends on | `__database_service` (`login_credential_table`) |
| Public API | `isAuthorized(user, pass)` — compares against the login table; `setAuthorized(bool)` / `getAuthorized()` — session flag used by CLI; `printConfigToTerminal` shows session name + cookie max-age |
| Used by | The web `AuthMiddleware` ([§9](#9-web-server-pending)) and the CLI's login prompt; the `auth` and `passwd` commands |
| Persistence | One row: `username`, `password`, `session_name`, `cookie_max_age` (default 300 s) |

#### 7.2.11 Storage (interface init, no provider)

`ENABLE_STORAGE_SERVICE` doesn't have its own `ServiceProvider` subclass — `PdiStack::initialize` simply calls `__i_fs.init()` ([PdiStack.cpp](src/PdiStack.cpp)). The filesystem is consumed directly by SSH/SFTP, the file-oriented CLI commands (`ls`/`cd`/`mv`/`cp`/`rm`/`mkd`/`mkf`/`cat`/`fwrite`/`head`/`tail`/`wc`/`df`/`grep`/`hexdump`), and any application code that wants to persist user files.

#### 7.2.12 `WebServer` — `__web_server`

| Flag | `ENABLE_HTTP_SERVER` (+ optional `ENABLE_HTTPS_SERVER` for TLS) |
|---|---|
| Source | [webserver/WebServer.{h,cpp}](src/webserver/WebServer.h) |
| Wired in PdiStack | `__web_server.initService(this->m_server)` where `m_server = &__i_http_server` |
| HTTP/HTTPS dispatch | If `ENABLE_HTTPS_SERVER` + `ENABLE_TLS_SERVICE` are on, `initService` calls `setServerCertificatePath` / `setServerPrivateKeyPath` (plus `setClientCertificateAuthorityPath` when `ENABLE_HTTPS_SERVER_MTLS`) using the defaults from [TlsConfig.h](src/config/TlsConfig.h), then `begin(HTTPS_DEFAULT_PORT=443, secure=true)`. Otherwise `begin(HTTP_DEFAULT_PORT=80)`. |
| Loop tick | `__web_server.handle_clients()` is called every `PdiStack::serve()` iteration |

Full breakdown lives in [§9. Web Server](#9-web-server) — it has its own router, middleware chain, controllers, views, and session handler that don't fit the `ServiceProvider` shape.

#### 7.2.13 `TelnetServiceProvider` — `__telnet_service`

| Flag | `ENABLE_TELNET_SERVICE` |
|---|---|
| Source | [transport/TelnetServiceProvider.{h,cpp}](src/service_provider/transport/TelnetServiceProvider.h) |
| Depends on | `iTcpServerInterface`, `__cmd_service`, `__auth_service` |
| Init does | Binds port 23 (configurable via `initService(&port)`), accepts one client at a time, hands its `iClientInterface*` to the CLI as a terminal |
| Public API | `start(port=23)`, `stop()`, `closeClient()`, `handle()` |
| Loop tick | Driven by `PdiStack::serve` indirectly through the scheduler |

#### 7.2.14 `SSHServer` — `__sshserver_service`

| Flag | `ENABLE_SSH_SERVICE` (⇒ `ENABLE_NETWORK_SERVICE` ∧ `ENABLE_STORAGE_SERVICE`) |
|---|---|
| Source | [shell/ssh/SSHServiceprovider.{h,cpp}](src/service_provider/shell/ssh/SSHServiceprovider.h) (in `LWSSH` namespace) |
| Depends on | `iTcpServerInterface`, `iFileSystemInterface`, crypto primitives ([src/utility/crypto/](src/utility/crypto/)), `__auth_service` |
| Public API | `start(port=22)`, `stop()`, `closeSession()`, `handle()`, `getSSHKeyPairs(algo, &pub, &priv, seedPlusPubFormat=false)` and per-protocol handlers (`handleVersionExchange`, `handleKeyExchange`, `handleAuthentication`, `handleChannelRequest`, `handleChannelSubsystemRequest`, `handleChannelSubsystemSftpRequest`, `handleChannelSftpBolusChunks`) |
| Key algorithms | See `SSHKeyAlgorithm` enum in [SshConfig.h](src/config/SshConfig.h) — Ed25519 / Curve25519-based |
| File transfer | SFTP subsystem implemented on top of the file system; supports interactive `sftp` (REALPATH, STAT, OPENDIR/READDIR, OPEN/READ/WRITE, MKDIR/RMDIR, REMOVE, RENAME, FSTAT; SETSTAT is no-op; READLINK/SYMLINK report unsupported) and SCP via `scp -s` (see README [SSH file transfer](#ssh-file-transfer)) |
| Cost | **Highest of any service** — keys, hash, symmetric AES, large per-session buffers |

#### 7.2.15 `CommandLineServiceProvider` — `__cmd_service`

| Flag | `ENABLE_CMD_SERVICE` |
|---|---|
| Source | [cmd/CommandLineServiceProvider.{h,cpp}](src/service_provider/cmd/CommandLineServiceProvider.h) |
| Inherits | `ServiceProvider`, `CommandExecutionInterface` |
| Depends on | `__auth_service`, every command in [cmd/commands/](src/service_provider/cmd/commands/) it dispatches to |
| Init does | Registers all command handlers; `CommandLineServiceProvider::startInteraction()` (called by PdiStack) attaches to the serial terminal and starts the login prompt |
| Public API | `useTerminal(iTerminalInterface*)` (swap the active terminal — used by telnet/SSH sessions), `processTerminalInput(t)`, `executeCommand(*cmd, inseq)`, `getCommandExecutedFromHistory(&out, idx, pattern)` |
| Built-in commands | `help`, `uptime`, `ls`, `cd`, `pwd`, `mkd`, `mkf`, `mv`, `cp`, `rm`, `cat`, `fwrite`, `head`, `tail`, `wc`, `df`, `grep`, `hexdump`, `cls`, `gpio`, `net`, `srvc`, `scht`, `ssh`, `tls` (if `ENABLE_TLS_CERT_GENERATION`), `iot`, `auth`, `reboot`, `watch` — full reference in [README Terminal table](#terminal) and [§10](#10-command-line--terminal) |
| Multi-terminal | The same command instance services serial, telnet, and SSH terminals — each just swaps the underlying `iTerminalInterface*` via `useTerminal` |

#### 7.2.16 TLS (no provider; transport hookup + cert provisioning)

`ENABLE_TLS_SERVICE` doesn't ship its own `ServiceProvider` either — it lives at the **interface + port** level and is consumed by anything that asks `iInstanceInterface` for a fresh client/server. The orchestrator's only direct involvement: when the flag is on, `PdiStack`'s shared HTTP client (`m_client`) is allocated via `__i_instance.getNewTlsClientInstance()` instead of TCP, so OTA / MQTT / Email / IoT / GPIO-post automatically run over TLS.

| Flag | `ENABLE_TLS_SERVICE` (+ `ENABLE_HTTPS_SERVER`, `ENABLE_HTTPS_SERVER_MTLS`, `ENABLE_TLS_CERT_GENERATION`, `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME`) |
|---|---|
| Backends | esp8266 → BearSSL ([devices/esp8266/TlsClientInterface.{h,cpp}](devices/esp8266/TlsClientInterface.h), [TlsServerInterface.{h,cpp}](devices/esp8266/TlsServerInterface.h), [BearSSLCertLoader.{h,cpp}](devices/esp8266/BearSSLCertLoader.h)). esp32 → mbedTLS ([devices/esp32/TlsClientInterface.{h,cpp}](devices/esp32/TlsClientInterface.h), [TlsServerInterface.{h,cpp}](devices/esp32/TlsServerInterface.h), [MbedTLSCertLoader.{h,cpp}](devices/esp32/MbedTLSCertLoader.h)) |
| Cert / key paths | All loaded from the FS at runtime via the per-port `*CertLoader` namespace. Defaults from [TlsConfig.h](src/config/TlsConfig.h): `/etc/http/server.crt`, `/etc/http/server.key`, `/etc/http/client-ca.crt`, `/etc/ssl/ca-bundle.crt`. |
| Threading | TLS handshakes (especially EC keygen / ECDSA sign) overflow the default Arduino cont_t stack on esp8266, so `ENABLE_TLS_SERVICE` forces `ENABLE_CONTEXTUAL_EXECUTION` and runs the BearSSL work on a dedicated cooperative task sized by `TLS_TASK_STACK_SIZE` (default 6500 B). |
| Outbound default | `PdiStack::PDIStack()` calls `setVerifyPeer(false)` on the bundled client so encrypted-but-unverified TLS works out-of-the-box. For production paths that cross an untrusted network, override with `setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH)` and remove the `setVerifyPeer(false)` line. |
| HTTPS server | [HttpServerInterfaceImpl](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h) carries a TLS branch (`begin(port, secure=true)`) that wraps each accepted client in the port's TLS server interface. Headers like `Strict-Transport-Security` are emitted when `HTTPS_HSTS_MAX_AGE_SECONDS` is non-zero. |
| Cert provisioning (esp32 only) | [devices/esp32/TlsCertProvisioner.{h,cpp}](devices/esp32/TlsCertProvisioner.h). `generateCert(certPath, keyPath, params)` issues a self-signed EC/RSA cert with optional CA bit, IPv4 / DNS SANs, custom validity. `ensureServerCert(certPath, keyPath, ip, dns)` creates the cert only if missing — wired to `EVENT_WIFI_STA_GOT_IP` when `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` is on. |
| Off-device cert generation | [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) — OpenSSL-backed alternative for boards without on-device generation (esp8266). Supports EC / RSA, multiple DNS / IP SANs, optional CA generation, and re-signing existing CSRs. Output goes under `certs/` and is uploadable to the device FS at the `TLS_DEFAULT_*_PATH` paths. |
| CLI | `tls q=1,t=<EC|RSA>,l=<bits>,n=<CN/DNS>,i=<IPv4>` — generates a server cert on-device (esp32 only). See [§10.7](#107-built-in-command-inventory). |

### 7.3 Init order and why it matters

`PDIStack::initialize` orders the calls deliberately:

```
1. database_service        ← every other service may need persisted config
2. serial_service          ← provides the boot terminal for log output
3. wifi_service            ← brings up the network
4. ota_service             ← may pre-empt all other startup if a new build is queued
5. gpio_service            ← can fire alerts via http/email once those exist
6. mqtt_service            ← may publish boot status
7. email_service           ← used by gpio events
8. factory_reset           ← always-on; final hook so all services have registered listeners
9. device_iot_service      ← needs MQTT already up
10. auth_service           ← gates everything CLI/Web sees
11. storage init           ← brings the FS up (needed by SSH)
12. web_server             ← needs auth + storage
13. telnet, ssh            ← need network + storage + cmd
14. cmd_service            ← started last so all `srvc` listings are complete
```

Two implications you cannot violate:
- A service initialised *later* can call into one initialised *earlier*; the reverse is undefined.
- A service that needs another service in its constructor will hit a static-init order trap. Defer all cross-service lookups to `initService` or runtime.

### 7.4 Cross-service event bus

Direct calls between services are reserved for known-earlier dependencies (OTA → device control). Anything fan-out happens through `__utl_event`. Common event names from [EventConfig.h](src/config/EventConfig.h):

| Event | Fired by | Typical listeners |
|---|---|---|
| `EVENT_FACTORY_RESET` | `__factory_reset.factory_reset()` | Database (clear tables), IoT (drop cache), GPIO (reset pins) |
| `EVENT_WIFI_STA_CONNECTED` / `_STA_DISCONNECTED` | `__wifi_service` | MQTT (re/dis-connect), OTA (suspend), IoT (suspend) |
| `EVENT_WIFI_STA_GOT_IP` | `__wifi_service` (after DHCP / static IP latches) | Cert provisioner (auto-mint with IP in SAN), services that need a stable address |
| `EVENT_WIFI_AP_STACONNECTED` / `_STADISCONNECTED` | `__wifi_service` | Captive-portal flows, per-client tracking |
| `EVENT_WIFI_INTERNET_UP` / `_DOWN` | `__wifi_service` connectivity poller | OTA, IoT, Email |
| `EVENT_GPIO_TRIGGER` | `__gpio_service` event detector | Email, MQTT, HTTP-post |
| `EVENT_SERIAL_AVAILABLE` | Serial input bridge | Sketch hooks |
| `EVENT_OTA_*` | `__ota_service` | Logger, web portal status |

Use `__utl_event.add_event_listener(name, [&](void* e){ … })` to subscribe; `__utl_event.fire(name, ptr)` to publish.

### 7.5 Writing a new service

To add (say) a `MetricsServiceProvider`:

1. **Pick a feature flag** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (`ENABLE_METRICS_SERVICE`) and a `service_t` enum value in [ServiceProvider.h](src/service_provider/ServiceProvider.h), guarded by the flag.
2. **Add the persisted struct** ([§4.4](#4-configuration-system)) and the DB table ([§6.9](#6-database-layer)).
3. **Create [src/service_provider/metrics/MetricsServiceProvider.{h,cpp}](src/service_provider/metrics/MetricsServiceProvider.h)** deriving from `ServiceProvider`:
   ```cpp
   MetricsServiceProvider() : ServiceProvider(SERVICE_METRICS, RODT_ATTR("Metrics")) {}
   bool initService(void* arg) override {
       __database_service.get_metrics_config_table(&m_cfg);
       m_service_routine_task_id = __task_scheduler.setInterval(
           [&]{ this->tick(); }, m_cfg.interval_ms, __i_dvc_ctrl.millis_now());
       return ServiceProvider::initService(arg);
   }
   ```
4. **Wire it in** the orchestrator: include guarded in [PdiStack.h](src/PdiStack.h), call `__metrics_service.initService(...)` from [PdiStack.cpp](src/PdiStack.cpp) in the right slot per [§7.3](#7-service-providers).
5. **Surface it**: implement `printConfigToTerminal` / `printStatusToTerminal` so `srvc` lists it; optionally add a CLI command under [cmd/commands/](src/service_provider/cmd/commands/) and a web controller under [webserver/controllers/](src/webserver/controllers/).
6. **Use events, not direct calls**, when you need to react to other services' state.

### 7.6 Gotchas

- **Constructor side-effects are a trap.** Allocating, reading the DB, or touching `__i_*` interfaces in your service's constructor will run before `setup()`, before the device's interface globals are initialised. Always defer to `initService`.
- **Don't `new` in `initService` either.** Use a value member or a static buffer; the embedded heap can fragment over months of uptime.
- **The base `stopService` clears `m_service_routine_task_id`** but does not cancel sub-tasks you scheduled. Track them and cancel explicitly in an override.
- **`SERVICE_DATABASE` and `SERVICE_FACTORY` are unconditional** in the `service_t` enum — every build always has them. Don't guard them with `ENABLE_*` even if your minimal build "doesn't need" them.
- **`printConfigToTerminal` is called from the CLI thread** (whichever lane the terminal is attached to). It must not block. If your config print is slow (e.g. iterates 50 MQTT topics), paginate via repeated `srvc` calls instead.
- **One global per service.** The `m_services[SERVICE_X]` slot is filled by the constructor; instantiating a second `XServiceProvider` overwrites the pointer and silently breaks `getService(SERVICE_X)`.

---

## 8. Transports

Transports are the **protocol implementations** that sit between the byte-level `iClientInterface` (which the device port provides) and the feature services (which deal in domain payloads). The three transports — HTTP, MQTT, SMTP — share one design: take a `iClientInterface*`, speak the wire protocol on it, hand the parsed result back to a service. They have **no device knowledge**, **no scheduling of their own**, and **no global state** other than per-instance buffers.

Implementation: [src/transports/](src/transports/). No global instances — each consumer creates an instance with an `iClientInterface*` it owns.

### 8.1 Why transports are separate from services

Three reasons the framework keeps these layers split:

1. **Reuse.** Both `WebServer` and `HTTP_CLIENT` need HTTP parsing; both `MqttServiceProvider` and `DeviceIotServiceProvider` need MQTT. A service owns *what to do*; the transport owns *how to speak the protocol*.
2. **Replaceability.** A device port that ships an SDK-native MQTT client can offer a `iClientInterface`-shaped wrapper and the framework's `MQTTClient` falls away — services don't change.
3. **Testability off-device.** Transports are pure C++ that take a stream — they can be exercised against a memory-backed `iClientInterface` mock without WiFi.

The service layer never opens its own socket: it asks `iInstanceInterface::getNewTcpClientInstance()` for a fresh `iTcpClientInterface*`, hands it to the transport, and lets the transport drive.

### 8.2 Transport summary

| Transport | Path | Class | Consumer services | Wire spec |
|---|---|---|---|---|
| HTTP | [transports/http/](src/transports/http/) | `Http_Client` | `OtaServiceProvider`, `DeviceIotServiceProvider`, `GpioServiceProvider` (HTTP-post mode), `WebServer` indirectly | HTTP/1.1 (default), 1.0 supported; placeholders for 2/3. HTTPS works through the same class — see [§8.3 HTTPS](#https--same-client-different-socket) |
| MQTT | [transports/mqtt/](src/transports/mqtt/) | `MQTTClient` (+ `mqtt_msg_*` builders) | `MqttServiceProvider`, `DeviceIotServiceProvider` | MQTT 3.1.1 |
| SMTP | [transports/smtp/](src/transports/smtp/) | `SMTPClient` | `EmailServiceProvider` | SMTP with `AUTH LOGIN`, no STARTTLS yet |

### 8.3 HTTP — `Http_Client`

Header: [HTTPClient.h](src/transports/http/HTTPClient.h). Two POD records — `http_req_t` and `http_resp_t` — carry request/response metadata; the class itself is the state machine that drives a connection through `iClientInterface`.

#### Lifecycle

```cpp
Http_Client http;
http.Begin();                                  // resets internal state
http.SetClient(__i_instance.getNewTcpClientInstance());
http.SetTimeout(HTTP_REQUEST_DURATION);        // 10000 ms by default
http.SetKeepAlive(true);
http.SetFollowRedirects(true);
http.SetRedirectLimit(3);
http.SetHttpVersion(HTTP_VERSION_1_1);
http.SetDefaultHeaders(true);                  // adds Host, User-Agent, Connection
http.AddReqHeader("X-Device", mac);

int16_t code = http.Get("http://api.example.com/v1/ping");
// or: http.Post(url, jsonBody);
// or: http.SendRequest("PATCH", url, body, len);

char *body; int16_t len;
http.GetResponse(body, len);                   // body is borrowed; do not free
char *agent;
http.GetRespHeader("Server", agent);

http.End(/*preserve_client=*/true);            // close session, optionally keep socket for next request
```

#### Key surface

| Member | Notes |
|---|---|
| `Begin` / `End(preserve_client)` / `ClearAll` | Session boundaries |
| `Connected()` | Status; lets the caller decide whether to reconnect |
| `SetUrl(url)` / `SetClient(c)` / `SetTimeout(ms)` / `SetKeepAlive(bool)` | Per-request knobs |
| `SetFollowRedirects(bool)` + `SetRedirectLimit(n)` | 3xx handling done inside the client |
| `SetUserAgent(s)` / `SetBasicAuthorization(u, p)` / `AddReqHeader(k, v, overwrite=true)` | Outgoing headers |
| `GetRespHeader(name, &value)` | Borrowed pointer; valid until `End`/next request |
| `Get(url)` / `Post(url, body)` / `SendRequest(method, url, body, size)` | Synchronous; return status code (negative on transport error) |
| `GetResponse(&body, &len)` | Response body buffer; bounded by `SetMaxRespBufferSize` |
| `SetHttpVersion(http_version_t)` | `HTTP_VERSION_1_0` / `_1_1` (defaults), `_2` / `_3` enums reserved |

#### Defaults (from [HttpConfig.h](src/config/HttpConfig.h))

| Macro | Value | Meaning |
|---|---|---|
| `HTTP_DEFAULT_PORT` | 80 | Used if URL omits port |
| `HTTPS_DEFAULT_PORT` | 443 | Used if URL omits port for `https://` |
| `HTTP_DEFAULT_VERSION` | `HTTP_VERSION_1_1` | Request line version |
| `HTTP_DEFAULT_KEEP_ALIVE_MS` | 30000 | Connection: keep-alive timeout sent to peer |
| `HTTP_REQUEST_DURATION` | 10000 (Common.h) | Per-request total budget |
| `HTTP_CLIENT_BUF_SIZE` | 640 | Working buffer used during parse |
| `HTTP_CLIENT_MAX_READ_MS` | 1500 | Per-chunk read timeout inside the parse loop |
| `HTTP_REQUEST_RETRY` | 1 | Connect retries before giving up |

#### Status codes

`http_resp_t::status_code` carries the standard HTTP code on success; on transport failure (connect/read/timeout) the `Get`/`Post`/`SendRequest` return is a negative `int16_t`. The full enum (`HTTP_RESP_*`) is in [HttpConfig.h](src/config/HttpConfig.h).

#### HTTPS — same client, different socket

`Http_Client` doesn't know or care whether its underlying `iClientInterface*` is plaintext TCP or TLS. To make any request HTTPS, only the factory call changes:

```cpp
Http_Client http;
http.Begin();

// HTTPS instead of HTTP:
iTlsClientInterface* tls = __i_instance.getNewTlsClientInstance();
tls->setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);   // optional
tls->setSNIHostname("api.example.com");                                  // recommended
http.SetClient(tls);

int16_t code = http.Get("https://api.example.com/v1/ping");
```

What you get for free: full handshake, peer-cert verification (when configured), record-level read/write, the same `GetResponse` / `GetRespHeader` parsing. What you should know:

- **The URL scheme is informational** — `Http_Client` doesn't open the socket, the client does. Passing a TLS client and a `http://...` URL still works (and travels encrypted); passing a TCP client and `https://...` connects in plaintext on the URL's port. Pair them correctly.
- **Default port follows the URL** — `https://` uses `HTTPS_DEFAULT_PORT = 443` from [HttpConfig.h](src/config/HttpConfig.h), `http://` uses `HTTP_DEFAULT_PORT = 80`.
- **`setVerifyPeer(false)` on the client** keeps the channel encrypted but skips chain validation — fine for dev / self-signed; **never** for production paths that cross an untrusted network.
- **`HTTPS_HSTS_MAX_AGE_SECONDS`** (server-side, [HttpConfig.h](src/config/HttpConfig.h)) is `0` by default — the HTTPS server doesn't emit `Strict-Transport-Security` until you set it non-zero (only do so when serving a CA-signed cert, otherwise the header hard-fails self-signed click-through).

The bundled `m_client` in `PdiStack` is already TLS-allocated when `ENABLE_TLS_SERVICE` is on (see [src/PdiStack.cpp](src/PdiStack.cpp)), so OTA, IoT, GPIO-post, and any other service that borrows that pointer automatically run over HTTPS without sketch changes.

#### Server-side HTTP / HTTPS

There is no `Http_Server` class in `transports/` — the **server** side lives at the interface layer ([`iHttpServerInterface`](src/interface/pdi/middlewares/iServerInterface.h)) with a portable default in [src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.{h,cpp}](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h). This split is intentional: the client is reused stand-alone; the server is plumbed through the `WebServer` ([§9](#9-web-server)) and never speaks bare HTTP.

HTTPS uses the **same** `HttpServerInterfaceImpl` — `begin(port, secure=true)` flips it into TLS mode, where each accepted client is wrapped via `__i_instance.getNewTlsServerInstance()` (so the request parser still sees decrypted bytes through the same `iClientInterface` API). Wiring details — cert/key path defaults, mTLS, HSTS — live in [§7.2.12](#7-service-providers) and [§7.2.16](#7-service-providers).

### 8.4 MQTT — `MQTTClient` + `mqtt_msg_*`

Two files split by concern:

- [Mqtt_msg.{h,cpp}](src/transports/mqtt/Mqtt_msg.h) — pure protocol encoder/decoder. Free functions like `mqtt_msg_publish(conn, topic, data, len, qos, retain, &msgId)` produce wire-format `mqtt_message_t` records from a fixed-size buffer. No I/O.
- [MqttClient.{h,cpp}](src/transports/mqtt/MqttClient.h) — `MQTTClient` class, the connection state machine: handshake, keep-alive, subscribe/publish, QoS-1/2 acknowledgement, callback dispatch.

#### Lifecycle

```cpp
MQTTClient mqtt;
mqtt.begin(client, &generalCfg, &lwtCfg);     // wires client + config tables
mqtt.OnConnected(&onMqttConnected);
mqtt.OnData(&onMqttData);                     // (args, topic, topic_len, data, len)
mqtt.OnDisconnected(&onMqttDisconnected);
mqtt.OnTimeout(&onMqttTimeout);

mqtt.InitConnection(host, MQTT_DEFAULT_PORT, /*security=*/0);
mqtt.InitClient(clientId, user, pass, /*keepAlive=*/60, /*cleanSession=*/1);
mqtt.InitLWT(willTopic, willMsg, /*qos=*/1, /*retain=*/0);

mqtt.Connect();
mqtt.Subscribe("ctrl/+/cmd", /*qos=*/1);
mqtt.Publish("sensor/temp", payload, len, /*qos=*/1, /*retain=*/0);
// ... mqtt.Disconnect() / mqtt.DeleteClient() during shutdown
```

#### Key surface

| Member | Notes |
|---|---|
| `begin(client, &general, &lwt)` | Single entry point; rest of the API just stores parameters |
| `InitConnection(host, port, security)` | `security` is a placeholder for TLS hooks per port |
| `InitClient(id, user, pass, keepAlive=60, cleanSession=1)` | Identity |
| `InitLWT(topic, msg, qos, retain)` | Will-message |
| `OnConnected/Disconnected/Published/Subscribed/Unsubscribed/Timeout(MqttCallback)` | `void(uint32_t* args)` |
| `OnData(MqttDataCallback)` | `void(uint32_t* args, const char* topic, uint32_t tlen, const char* data, uint32_t dlen)` |
| `Subscribe(topic, qos)` / `UnSubscribe(topic)` | Returns success of *enqueuing*, not of ack |
| `Connect()` / `Disconnect()` | State transitions |
| `Publish(topic, data, len, qos, retain)` | QoS 0/1/2 |
| `DeleteClient()` | Releases internal buffers; does not free `iClientInterface*` |

#### Callbacks and threading

All callbacks fire on whatever lane drives `MqttServiceProvider::handleMqttPublish` / `handleMqttSubScribe`. By default that is the inline scheduler. Don't block in a callback — schedule any expensive work as a follow-up via `__task_scheduler.setTimeout(..., 1, now())`, exactly as the `DeviceIotServiceProvider` does it (see [DeviceIotServiceProvider.cpp grep](src/service_provider/iot/DeviceIotServiceProvider.cpp)).

#### Defaults

| Macro | Value |
|---|---|
| `MQTT_DEFAULT_PORT` | 1883 |
| `MQTT_DEFAULT_KEEPALIVE` | 60 s |

Service-level tables (`mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table`) live in [MqttConfig.h](src/config/MqttConfig.h); the `MqttServiceProvider` translates them into the `Init*` calls above.

#### Direct use of `mqtt_msg_*`

Useful when you need to encode a packet without owning a connection — e.g. SSH-tunnelled MQTT, or a unit test. `mqtt_msg_init(&conn, buf, size)` binds a buffer; subsequent `mqtt_msg_connect`, `mqtt_msg_publish`, `mqtt_msg_subscribe`, etc. return a `mqtt_message_t*` whose `data`/`length` you write to whatever stream you have. `mqtt_get_total_length` and `mqtt_get_id` parse incoming frames.

### 8.5 SMTP — `SMTPClient`

Implementation: [SMTPClient.{h,cpp}](src/transports/smtp/SMTPClient.h). A blocking, command-response client built on top of `iClientInterface`. Each `sendXxx` issues one SMTP verb and waits for the expected reply code (or arbitrary expected string).

#### Lifecycle

```cpp
SMTPClient smtp;
smtp.begin(client, host, port);

smtp.sendHello(domain);                    // EHLO/HELO
smtp.sendAuthLogin(username, password);    // AUTH LOGIN (base64 user, base64 pass)
smtp.sendFrom(sender);                     // MAIL FROM:<...>
smtp.sendTo(recipient);                    // RCPT TO:<...>
smtp.sendDataCommand();                    // DATA
smtp.sendDataHeader(sender, recipient, subject);
smtp.sendDataBody(body);                   // string overloads
smtp.sendQuit();                           // QUIT
smtp.end();
```

#### Key surface

| Member | Notes |
|---|---|
| `begin(client, host, port)` | Stores parameters; **does not** connect — the first `sendXxx` does |
| `readResponse()` / `flushClient()` | Low-level: read a single CRLF-terminated reply, drain pending bytes |
| `startReadResponse(timeOut=5000)` / `waitForResponse(timeOut)` | Block until a reply is available |
| `waitForExpectedResponse(expected, timeOut)` | Returns true if the reply equals `expected` |
| `sendCommandAndExpect(cmd, expected, timeOut)` | Combined send + match |
| `sendCommandAndGetCode(cmd, timeOut)` | Returns the 3-digit reply code as `int` |
| `sendHello`, `sendAuthLogin`, `sendFrom`, `sendTo`, `sendDataCommand`, `sendDataHeader`, `sendDataBody`, `sendQuit` | Convenience wrappers around the verbs above |
| `end()` | Disconnect; does not free the `iClientInterface*` |

#### Defaults & status

| Macro / type | Value |
|---|---|
| `SMTP_DEFAULT_TIMEOUT` | 5000 ms (`MILLISECOND_DURATION_5000`) |
| `enum smtp_reply_code` | Standard reply codes (220, 250, 334, 354, 235, 221, …) |
| `enum smtp_command_status` | Success/failure of each helper |

#### Limitations to know

- **No STARTTLS.** SMTP is plaintext on whatever port you give it; for TLS-only providers you need a port that supports TLS at the `iClientInterface` level.
- **No multi-recipient batching.** Loop `sendTo` per recipient between `MAIL FROM` and `DATA`.
- **Blocking.** All verbs synchronously block on the wire — call from a service tick that already accepts a multi-second tail latency. `EmailServiceProvider::sendMail` schedules this off the critical path.
- **No body encoding.** The transport doesn't quoted-printable / base64 the body. If you need attachments or non-ASCII, encode at the caller.

### 8.6 Helpers (`src/helpers/`)

Two small shims live alongside the transports to keep service code terse:

- [ClientHelper.{h,cpp}](src/helpers/ClientHelper.h) — `connectToServer`, `disconnect`, `isConnected`, `sendPacket`, `readPacket`. These work on any `iClientInterface*` and add timeout discipline + chunked write/read. Anyone using a transport at the byte level (e.g. SSH tunnel) reaches for these instead of the raw interface.
- [HttpHelper.h](src/helpers/HttpHelper.h) — `getHttpStatusString(code)`, `getMimeTypeString(mimetype_t)`, `getHttpMethodString(http_method_t)`, plus the static `HTTP_SERVER_DEFAULT_STATIC_PATH = "/var/www/static/"`. Used by `Http_Client`, `WebServer`, and the web controllers.
- [StorageHelper.h](src/helpers/StorageHelper.h) — `getMimeTypeString` / `getMimeTypeExtension` for filename ↔ MIME mapping; pulled in by the FS CLI and SFTP.

### 8.7 Adding a new transport

If you want to add (say) CoAP:

1. **Create [src/transports/coap/CoAPClient.{h,cpp}](src/transports/coap/CoAPClient.h)** that takes a `iClientInterface*` (UDP-shaped, so the port must expose UDP via the same interface) and exposes a `begin/send/receive` API plus callbacks for incoming responses.
2. **Keep it free of `__database_service` and `__task_scheduler`.** Transports do not own time or persistence — they parse bytes.
3. **Build the service on top** under `src/service_provider/transport/CoAPServiceProvider.{h,cpp}` — that's the layer that owns config, scheduling, and event dispatch.
4. **Wire one new flag** (`ENABLE_COAP_SERVICE`) in [DeviceConfig.h](devices/DeviceConfig.h), guard the orchestrator wiring in [PdiStack.h](src/PdiStack.h) / [PdiStack.cpp](src/PdiStack.cpp), and you're done.

### 8.8 Gotchas

- **Transports don't own the `iClientInterface*`.** Whoever called `getNewTcpClientInstance()` must `delete` it after `End`/`Disconnect`/`end`. Forgetting leaks one socket per request — fatal on long-uptime nodes.
- **No retry loops in transports.** `HTTP_REQUEST_RETRY = 1` covers connect; everything else is a single attempt. Retries belong to the service layer (which knows whether the failure is recoverable).
- **HTTP response body is a borrowed pointer.** `GetResponse(&body, &len)` returns an internal buffer; if you need it past the next request, copy.
- **MQTT `Subscribe` returns true on enqueue, not on SUBACK.** Wait for `OnSubscribed` (or skip waiting and accept best-effort) before publishing on the freshly-subscribed topic.
- **SMTP timeouts are per-verb.** A flaky link can sum up to `n × SMTP_DEFAULT_TIMEOUT` for one email; bound the service-side budget explicitly.
- **HTTP/2 and /3 enums exist; the implementation does not.** Setting `HTTP_VERSION_2` silently degrades to 1.1. Don't ship code that depends on multiplexed requests until the impl lands.

---

## 9. Web Server

The web server is the **on-device admin portal**. When `ENABLE_HTTP_SERVER` is on, every device runs an HTTP/1.1 server on its access-point IP (`192.168.0.1` by default) that lets a phone or laptop log in, configure WiFi, drive GPIO pins, watch the dashboard, upload files, and manage MQTT/OTA/Email/IoT — without flashing or serial cables. With `ENABLE_HTTPS_SERVER` (+ `ENABLE_TLS_SERVICE`) the same portal serves on port 443 with TLS-wrapped sockets — no controller / page / route changes, just a different `begin()` call and a cert/key on disk. See [§9.7.1 HTTPS wiring](#971-https-wiring--certificates) for the full flow.

Structurally it is the framework's largest subsystem (≈7 KLOC), but it follows a small handful of concepts: an `HttpServer` orchestrator, a `RouteHandler` that maps URIs to callbacks, a `Middleware` chain (currently mostly auth), a `WebResourceProvider` that owns the response buffer and the static fragments, per-feature `Controller`s, and a set of HTML page templates broken into header/body/footer.

Implementation: [src/webserver/](src/webserver/). Global instance: `__web_server`. Wired into `PdiStack::initialize` ([PdiStack.cpp](src/PdiStack.cpp)) and ticked from `PdiStack::serve` via `__web_server.handle_clients()` ([PdiStack.cpp](src/PdiStack.cpp)).

### 9.1 Layered model

```
              ┌────────────────────────────────────────────┐
  iHttpServer │  iHttpServerInterface  (port: §3.3.3)      │  ← bytes in/out, route registration
  Interface   │  default: HttpServerInterfaceImpl          │
              └──────────────────────┬─────────────────────┘
                                     │  on(uri, cb), arg(), header(), send()
              ┌──────────────────────▼─────────────────────┐
              │  HttpServer  ─ owns one instance of every  │  ← src/webserver/WebServer.{h,cpp}
              │  *Controller, ENABLE_*-gated.              │     (subclasses ServiceProvider)
              └──────────────────────┬─────────────────────┘
                                     │  controllers self-boot
              ┌──────────────────────▼─────────────────────┐
              │  Controller (base)  +  HomeController,     │  ← src/webserver/controllers/
              │  LoginController, DashboardController,     │
              │  OtaController, WiFiConfigController,      │
              │  GpioController, MqttController,           │
              │  EmailConfigController, DeviceIotController│
              │  StorageController                          │
              └────────┬──────────────────────────┬────────┘
                       │ register_route(uri, fn,  │  build response via
                       │   middleware, redirect)  │  m_web_resource & helpers
                       ▼                          ▼
              ┌──────────────────────┐  ┌──────────────────────────────────┐
              │  RouteHandler        │  │  WebResourceProvider (m_web_     │  ← resources/
              │  __web_route_handler │  │  resource): page buffer, server  │
              └──────────┬───────────┘  │  ptr, DB accessors, MIME helpers │
                         │              └────────────────┬─────────────────┘
                         ▼                               │
              ┌──────────────────────┐                   │  emits HTML using
              │  Middleware          │                   ▼
              │  ├─ AUTH_MIDDLEWARE  │   ┌──────────────────────────────────┐
              │  │   (session check) │   │  pages/  (Header, Footer,        │
              │  └─ EwSessionHandler │   │  HomePage, Dashboard, LoginPage, │
              │      (cookie ops)    │   │  *ConfigPage, StorageListPage,   │
              └──────────────────────┘   │  NotFound, LogoutPage)           │
                                         │  helpers/  (DynamicPageBuild     │
                                         │  Helper, HtmlTagsAndAttr,        │
                                         │  icon/SvgIcons)                  │
                                         └──────────────────────────────────┘
```

Reading the diagram: a request hits the port (`iHttpServerInterface::handleClient` ⇒ matches a URI to a registered lambda) ⇒ the lambda runs middleware ⇒ on pass, the controller method runs, pulls config from `__database_service`, composes HTML using header/page/footer fragments + `DynamicPageBuildHelper`, and `send`s through the server. On fail, middleware sends a `301` to `/login`.

### 9.2 `HttpServer` orchestrator

[WebServer.h](src/webserver/WebServer.h). Despite its name, this class is a `ServiceProvider` ([§7.2.12](#7-service-providers)) — but unlike the others, it directly owns its sub-objects as value members rather than registering as singletons elsewhere.

| Member | Always present | `ENABLE_*` gated |
|---|---|---|
| `HomeController m_home_controller` | yes | — |
| `DashboardController m_dashboard_controller` | yes | — |
| `LoginController m_login_controller` | yes | — |
| `OtaController m_ota_controller` | | `ENABLE_OTA_SERVICE` |
| `WiFiConfigController m_wificonfig_controller` | | `ENABLE_WIFI_SERVICE` |
| `GpioController m_gpio_controller` | | `ENABLE_GPIO_SERVICE` |
| `MqttController m_mqtt_controller` | | `ENABLE_MQTT_SERVICE` |
| `EmailConfigController m_emailconfig_controller` | | `ENABLE_EMAIL_SERVICE` |
| `DeviceIotController m_device_iot_controller` | | `ENABLE_DEVICE_IOT` |
| `StorageController m_storage_controller` | | `ENABLE_STORAGE_SERVICE` |

`initService(iHttpServerInterface*)` stashes the server pointer, attaches it to `__web_resource.m_server`, and iterates the static `Controller::m_controllers` registry calling each `controller->boot()`. Every controller's constructor pushes itself onto that registry (see [§9.4](#9-web-server)), so booting is automatic at static-init plus one pass at `initService` time.

The final step in `initService` is the `begin()` call — and this is where the HTTPS branch lives:

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

No controller, page, route, middleware, or session code is conditional on TLS — the only thing the build flips is which `begin()` overload runs at the bottom of `initService`. Cert/key paths are macros from [TlsConfig.h](src/config/TlsConfig.h) so a sketch can override them by `#define`-ing before any framework include.

`handle_clients()` is a one-liner: `m_server->handleClient()`. All actual work happens inside the route lambdas the controllers registered.

### 9.3 Route registry — `RouteHandler` + `Routes.h`

URIs are constants in [routes/Routes.h](src/webserver/routes/Routes.h). The full set:

| Route | Purpose | Middleware |
|---|---|---|
| `/` | Home / menu landing | None |
| `/login` | Login form + POST handler | None |
| `/logout` | Drop session, redirect to `/login` | AUTH |
| `/login-config` | Change username/password | AUTH |
| `/dashboard` | Live device summary | AUTH |
| `/listen-dashboard` | Long-poll/SSE-style dashboard updates | AUTH |
| `/wifi-config` | WiFi STA/AP form | AUTH |
| `/ota-config` | OTA host/port/version | AUTH |
| `/email-config` | SMTP credentials | AUTH |
| `/gpio-manage` | Per-pin GPIO panel | AUTH |
| `/gpio-server`, `/gpio-config`, `/gpio-write`, `/gpio-event` | GPIO subforms | AUTH |
| `/gpio-monitor`, `/listen-monitor` | Live GPIO graphs (analog) | AUTH |
| `/mqtt-manage`, `/mqtt-general-config`, `/mqtt-lwt-config`, `/mqtt-pubsub-config` | MQTT subforms | AUTH |
| `/device-register-config` | IoT register / OTP | AUTH |
| `/storage`, `/storage-fileupload`, `/storage-filelist`, `/storage-filedel` | File browser & upload | AUTH |

[RouteHandler](src/webserver/handlers/RouteHandler.h) extends `Middleware` and adds two methods:

```cpp
void register_route(const char* uri,
                    CallBackVoidArgFn fn,
                    middlwares level = NO_MIDDLEWARE,
                    const char* redirect = WEB_SERVER_LOGIN_ROUTE);
void register_not_found_fn(CallBackVoidArgFn fn);
```

Global instance: `__web_route_handler`. Controllers reach it via `m_route_handler` (a base-class pointer set in `Controller`'s ctor).

### 9.4 `Controller` base class and the controller registry

[Controller.h](src/webserver/controllers/Controller.h). Each controller:

- Inherits `Controller`, which on construction pushes `this` into the static `Controller::m_controllers` vector.
- Holds protected pointers `m_web_resource` (the response buffer / page helpers) and `m_route_handler` (the global route registry).
- Implements `boot()` to register its routes.

A typical controller body (paraphrased from [LoginController.h](src/webserver/controllers/LoginController.h)):

```cpp
class LoginController : public Controller {
public:
    LoginController() : Controller("LoginController") {}

    void boot() override {
        m_route_handler->register_route(
            WEB_SERVER_LOGIN_ROUTE,
            [&] { this->handleLoginRoute(); });

        m_route_handler->register_route(
            WEB_SERVER_LOGOUT_ROUTE,
            [&] { this->handleLogoutRoute(); });

        m_route_handler->register_route(
            WEB_SERVER_LOGIN_CONFIG_ROUTE,
            [&] { this->handleLoginConfigRoute(); },
            AUTH_MIDDLEWARE);                    // gated
    }

private:
    void handleLoginRoute();
    void handleLogoutRoute();
    void handleLoginConfigRoute();
};
```

#### Controller inventory

| Controller | Source | Routes owned |
|---|---|---|
| `HomeController` | [HomeController.h](src/webserver/controllers/HomeController.h) | `/`, plus the `register_not_found_fn` 404 handler |
| `LoginController` | [LoginController.h](src/webserver/controllers/LoginController.h) | `/login`, `/logout`, `/login-config` |
| `DashboardController` | [DashboardController.h](src/webserver/controllers/DashboardController.h) | `/dashboard`, `/listen-dashboard` |
| `WiFiConfigController` | [WiFiConfigController.h](src/webserver/controllers/WiFiConfigController.h) | `/wifi-config` |
| `OtaController` | [OtaController.h](src/webserver/controllers/OtaController.h) | `/ota-config` |
| `GpioController` | [GPIOController.h](src/webserver/controllers/GPIOController.h) (~900 lines) | All `/gpio-*` |
| `MqttController` | [MQTTController.h](src/webserver/controllers/MQTTController.h) (~560 lines) | All `/mqtt-*` |
| `EmailConfigController` | [EmailConfigController.h](src/webserver/controllers/EmailConfigController.h) | `/email-config` |
| `DeviceIotController` | [DeviceIotController.h](src/webserver/controllers/DeviceIotController.h) | `/device-register-config` |
| `StorageController` | [StorageController.h](src/webserver/controllers/StorageController.h) | All `/storage-*` |

### 9.5 Middleware

[Middleware.h](src/webserver/middlewares/Middleware.h). Three levels:

| Level | Action |
|---|---|
| `NO_MIDDLEWARE` | Pass straight through to the route handler |
| `AUTH_MIDDLEWARE` | Check `EwSessionHandler::has_active_session()`; on fail, send `301` with `Location: <redirect>` (default `/login`) and abort the route |
| `API_MIDDLEWARE` | Currently a no-op stub — slot reserved for API-key/HMAC validation |

The middleware machinery extends `EwSessionHandler` rather than being a separate chain. Adding a new middleware level means: extend the `middlwares` enum, add an `else if` branch in `handle_middleware`, and route handlers can opt in with the new value.

### 9.6 Sessions — `EwSessionHandler`

[SessionHandler.h](src/webserver/handlers/SessionHandler.h). The framework's session model is **cookie-based**:

- Session name comes from `login_credential_table::session_name` (default `pdi_session` from [ServerConfig.h](src/config/ServerConfig.h)).
- Cookie max-age comes from `login_credential_table::cookie_max_age` (default 300 s).
- `EW_COOKIE_BUFF_MAX_SIZE = 60` is the working buffer used to read/parse the incoming `Cookie:` header.
- `create_session()` builds a `Set-Cookie` header with a derived token and writes it on the login response.
- `has_active_session()` parses the request's `Cookie:` header against the persisted credentials.
- `send_inactive_session_headers()` overwrites with an expired cookie — used on `/logout` and 301 redirects.

The session token is derived per-login rather than rotated periodically; a logout invalidates it on the client only. There is no server-side session table — the auth layer is intentionally minimal.

### 9.7 Response composition — `WebResourceProvider` and pages

[WebResource.h](src/webserver/resources/WebResource.h). Holds:

- `iHttpServerInterface* m_server` — the current request/response context.
- The page buffer (`PAGE_HTML_MAX_SIZE = 1800` bytes — yes, **per-page HTML is capped at ~1.8 KB**; bigger pages stream in chunks via `send(..., send_in_chunks=true)`).
- Accessor shortcuts into `__database_service` so controllers don't need to import the DB headers.

`collect_resource(iHttpServerInterface*)` is called from `HttpServer::initService` to stash the server pointer that controllers will reach through `m_web_resource->m_server` for `arg()`, `hasArg()`, `header()`, `addHeader()`, `send()`.

#### Pages

[src/webserver/pages/](src/webserver/pages/) ships HTML fragments as raw C strings declared in `PROG_RODT_ATTR` (program memory). The composition pattern is fixed three-part:

```cpp
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml,  /*chunked=*/true);
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, bodyHtml,    /*chunked=*/true);
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml,  /*chunked=*/true);
```

[Header.h](src/webserver/pages/Header.h) and [Footer.h](src/webserver/pages/Footer.h) are the framing every page reuses. The middle slot is one of [HomePage.h](src/webserver/pages/HomePage.h), [Dashboard.h](src/webserver/pages/Dashboard.h), [LoginPage.h](src/webserver/pages/LoginPage.h), [WiFiConfigPage.h](src/webserver/pages/WiFiConfigPage.h), [OtaConfigPage.h](src/webserver/pages/OtaConfigPage.h), [GpioConfigPage.h](src/webserver/pages/GpioConfigPage.h), [MqttConfigPage.h](src/webserver/pages/MqttConfigPage.h), [EmailConfigPage.h](src/webserver/pages/EmailConfigPage.h), [DeviceIotPage.h](src/webserver/pages/DeviceIotPage.h), [LoginConfigPage.h](src/webserver/pages/LoginConfigPage.h), [LogoutPage.h](src/webserver/pages/LogoutPage.h), [StorageListPage.h](src/webserver/pages/StorageListPage.h), [NotFound.h](src/webserver/pages/NotFound.h).

#### Helpers

| Helper | Purpose |
|---|---|
| [DynamicPageBuildHelper.h](src/webserver/helpers/DynamicPageBuildHelper.h) | Convert C structs into HTML form inputs (text/number/checkbox/select), generate tables, escape values, build option lists — the bulk of dynamic markup |
| [HtmlTagsAndAttr.h](src/webserver/helpers/HtmlTagsAndAttr.h) | String constants for common tag/attr names — keeps allocations down |
| [icon/SvgIcons.h](src/webserver/helpers/icon/SvgIcons.h) | Inline SVGs used in the menu / dashboard |

#### 9.7.1 HTTPS wiring & certificates

When `ENABLE_HTTPS_SERVER` is on alongside `ENABLE_TLS_SERVICE`, the **same** `HttpServerInterfaceImpl` ([src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.{h,cpp}](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h)) runs in TLS mode. The split of responsibility is:

| Layer | What it does in HTTPS mode |
|---|---|
| `HttpServer::initService` ([§9.2](#92-httpserver-orchestrator)) | Calls `setServerCertificatePath` / `setServerPrivateKeyPath` (+ `setClientCertificateAuthorityPath` if `ENABLE_HTTPS_SERVER_MTLS`), then `begin(HTTPS_DEFAULT_PORT, secure=true)`. Paths come from [TlsConfig.h](src/config/TlsConfig.h) macros. |
| `iHttpServerInterface` ([§3.3.3](#3-portable-interfaces)) | Adds the TLS-only methods `setServerCertificatePath`, `setServerPrivateKeyPath`, `setClientCertificateAuthorityPath`, and the `secure` arg on `begin(port, secure)`. All three setters are no-op stubs in the non-TLS build, so existing ports compile unchanged. |
| `HttpServerInterfaceImpl::begin(port, secure=true)` | Switches the listener over to `__i_instance.getNewTlsServerInstance()`. Each accepted client is a `iTlsClientInterface*` masquerading as the same `iClientInterface` shape the request parser already uses — so handler code never sees raw TLS frames. |
| Port-level TLS backend | esp8266 → BearSSL, esp32 → mbedTLS — loads PEM/DER from the FS at the configured paths via `TlsCryptoLoader` ([devices/esp8266/BearSSLCertLoader.{h,cpp}](devices/esp8266/BearSSLCertLoader.h), [devices/esp32/MbedTLSCertLoader.{h,cpp}](devices/esp32/MbedTLSCertLoader.h)). |
| Cert provisioning | Two paths — on-device `tls q=1,t=…,n=…,i=…` CLI command (esp32 only, gated on `ENABLE_TLS_CERT_GENERATION`; auto-mint at first STA-got-IP if `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME`); off-device [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) for esp8266. See [§7.2.16](#7-service-providers). |

Required filesystem layout (defaults from [TlsConfig.h](src/config/TlsConfig.h) — overridable per project):

| Path macro | Default | Purpose |
|---|---|---|
| `TLS_DEFAULT_SERVER_CERT_PATH` | `/etc/http/server.crt` | HTTPS server certificate (PEM; may be a chain) |
| `TLS_DEFAULT_SERVER_KEY_PATH` | `/etc/http/server.key` | HTTPS server private key (PEM; EC or RSA) |
| `TLS_DEFAULT_CLIENT_CA_PATH` | `/etc/http/client-ca.crt` | CA bundle for mTLS client-cert verification (only if `ENABLE_HTTPS_SERVER_MTLS`) |

Upload these via SFTP ([§7.2.14](#7-service-providers)) after first boot, then `reboot` — the HTTPS listener picks them up on the next `initService` pass. The storage service creates `/etc/http/` lazily; you don't need to `mkd` it ahead of time.

HTTPS-specific response headers worth knowing:

- **`Strict-Transport-Security`** — emitted by the HTTPS server when `HTTPS_HSTS_MAX_AGE_SECONDS` ([HttpConfig.h](src/config/HttpConfig.h)) is non-zero. Default is **0** (header not sent). Turn on only with a CA-signed cert; with a self-signed cert HSTS hard-fails the browser click-through and locks the portal out.

### 9.8 Request lifecycle (end to end)

```
client request
    │
    ▼
iHttpServerInterface::handleClient()
    │  parse method + URI + headers + args
    │
    ▼
matching route lambda registered in __web_route_handler
    │  (the lambda was captured by Controller::boot())
    │
    ▼
Middleware::handle_middleware(level, redirect)
    │  if AUTH and !has_active_session(): send 301 → /login, return
    │
    ▼
Controller::handleXxxRoute()
    │  read args/headers via m_web_resource->m_server->arg(name)
    │  pull config: __database_service.get_<x>_config_table(&cfg)
    │  on POST: validate, set table, optionally fire event / restart service
    │  build HTML: header + page + footer through send(..., chunked=true)
    │
    ▼
iHttpServerInterface::send(code, mime, body, chunked)
    │
    ▼
bytes back to client
```

### 9.9 Per-route data flow examples

**`/wifi-config` POST** (from [WiFiConfigController.h](src/webserver/controllers/WiFiConfigController.h)):
1. Middleware: AUTH passes.
2. Read `arg("sta_ssid")`, `arg("sta_password")`, AP fields.
3. `__database_service.get_wifi_config_table(&cur)` to preserve untouched fields.
4. Apply args to the struct.
5. `__database_service.set_wifi_config_table(&cur)`.
6. Emit success page; schedule `__wifi_service.configure_wifi_station(...)` via `__task_scheduler.setTimeout(..., 1)` so the response flushes before the radio drops.
7. Device reconnects with new credentials.

**`/listen-monitor` (analog GPIO live graph)**:
- Browser issues repeated GETs (long-poll style); each returns the latest analog sample as JSON.
- `GpioController::handleAnalogMonitor` reads the pin via `__i_dvc_ctrl.gpioRead(ANALOG_READ, pin)`, builds `{ "p":N, "v":val }`, sends.

**`/storage-fileupload` (multipart)**:
- `StorageController` reads the upload via the server's chunked body API, writes to the file system via `__i_fs.createFile/appendFile`.
- Filename is sanitised against `MIN_ACCEPTED_ARG_SIZE`.

### 9.10 Adding a new page

To add a `/metrics` page that shows live counters:

1. **Add the route constant** in [routes/Routes.h](src/webserver/routes/Routes.h):
   ```c
   #define WEB_SERVER_METRICS_ROUTE "/metrics"
   ```
2. **Create `MetricsPage.h`** under [src/webserver/pages/](src/webserver/pages/) with the body HTML as a `PROG_RODT_ATTR` const.
3. **Create `MetricsController`** under [src/webserver/controllers/](src/webserver/controllers/):
   ```cpp
   class MetricsController : public Controller {
   public:
       MetricsController() : Controller("MetricsController") {}
       void boot() override {
           m_route_handler->register_route(
               WEB_SERVER_METRICS_ROUTE,
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
4. **Add a value-member** in [WebServer.h](src/webserver/WebServer.h) (under the matching `ENABLE_*` if optional), plus `#include` of the controller header. No registry plumbing — the base-class constructor self-registers.
5. **Add a menu entry** in the existing home/header page so users can navigate to it.

### 9.11 Gotchas

- **Page size cap (`PAGE_HTML_MAX_SIZE = 1800`).** A single non-chunked `send` is truncated to 1.8 KB. Always pass `send_in_chunks=true` for composed pages, and split big HTML into header/body/footer fragments.
- **Cookie storage is single-row.** Only one logged-in user at a time is well-supported; concurrent sessions reuse the same cookie token.
- **`API_MIDDLEWARE` is a stub.** Don't assume `API_MIDDLEWARE` enforces anything — if you're shipping an API surface, add the check now.
- **Page strings live in flash (`PROG_RODT_ATTR`).** Don't `strcpy` them into RAM unless you have to; pass them straight to `send()` which handles flash-string reads.
- **All controller routes go through one global `RouteHandler`.** Two controllers registering the same URI silently overwrite each other on the underlying `iHttpServerInterface::on()` — make URI constants unique.
- **`handle_clients` is called every `serve()` tick.** Long controller methods stall the entire main loop. If a route needs >100 ms of work (e.g. mass file delete), schedule it via `__task_scheduler.setTimeout(..., 1, now())` and return a 202 immediately.
- **No CSRF protection.** Forms rely on session cookies only; same-origin assumed. Don't expose the portal on the WAN.
- **Static-init order across controllers.** `Controller::m_controllers` is a static vector; controllers constructed during static init push into it. If your controller's constructor side-effects depend on another global, defer to `boot()` — which only runs from `initService`.
- **HTTPS or HTTP, not both.** The current `WebServer::initService` calls `begin()` exactly once — either on `HTTPS_DEFAULT_PORT (443)` with `secure=true` or on `HTTP_DEFAULT_PORT (80)`. Building with `ENABLE_HTTPS_SERVER` flips the whole portal to TLS; there's no parallel plaintext listener. If you need both (e.g. redirect 80 → 443), drive a second `iTcpServerInterface` from a sketch.
- **HTTPS fails closed if cert/key are missing.** With `ENABLE_HTTPS_SERVER`, the listener refuses connections (or never starts, depending on the port) when `/etc/http/server.crt` / `/etc/http/server.key` aren't on the FS. Provision before flipping the flag — or use `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` on esp32 so an `EVENT_WIFI_STA_GOT_IP` listener auto-mints a self-signed pair on first boot.
- **Self-signed cert + browser HSTS is a foot-gun.** Default `HTTPS_HSTS_MAX_AGE_SECONDS = 0` keeps the header off. If you turn HSTS on while serving a self-signed cert, the browser pins HTTPS-only for the configured age and hard-fails the click-through — you'll lose portal access until the pin expires or you clear it manually.
- **Session cookie isn't marked `Secure`.** The framework's session cookie ([§9.6](#96-sessions--ewsessionhandler)) doesn't carry the `Secure` attribute even under HTTPS, so it'll fall through plaintext if you ever ran the portal on HTTP. After switching to HTTPS, factory-reset the credential row (or change the session name) to invalidate any HTTP-issued cookies still in client caches.

---

## 10. Command Line / Terminal

The CLI is the framework's universal control plane. The same set of commands is reachable over **serial** by default, **telnet** on port 23, and **SSH** on port 22 — with login, command history, autocomplete, in-place editing, and a SFTP file-transfer subsystem. The trick that makes one implementation cover three channels: every terminal source surfaces as an `iTerminalInterface*`, and the CLI rebinds that pointer per session.

Implementation: [src/service_provider/cmd/](src/service_provider/cmd/) (`__cmd_service`) plus the parser primitives in [src/utility/CommandBase.{h,cpp}](src/utility/CommandBase.h). User-facing command table is in the main README's [Terminal section](#terminal).

### 10.1 Layered model

```
  Application stream    Serial port   Telnet TCP session   SSH channel
                            │                │                  │
                            └────────────────┴──────────────────┘
                                             │  iTerminalInterface*
                                             ▼
                            ┌──────────────────────────────────────┐
                            │  CommandLineServiceProvider          │  ← src/service_provider/cmd/
                            │  __cmd_service                       │
                            │  ─ owns the input ring buffer        │
                            │  ─ binds one terminal at a time      │
                            │  ─ history + autocomplete + editing  │
                            └──────────┬───────────────────────────┘
                                       │ dispatch
                                       ▼
                            ┌──────────────────────────────────────┐
                            │  CommandBase registry                │  ← src/utility/CommandBase.h
                            │  static vector<CommandProp>          │     +  src/service_provider/cmd/commands/
                            │  ─ each command self-registers       │
                            │  ─ parses `cmd opt=val[,opt=val]…`   │
                            │  ─ supports free-arg, holding opts,  │
                            │    waiting-for-input, abort signals  │
                            └──────────┬───────────────────────────┘
                                       │ delegate to service
                                       ▼
                            ┌──────────────────────────────────────┐
                            │  Service Providers (§7)              │
                            │  __wifi_service, __gpio_service,     │
                            │  __task_scheduler, __i_fs, …         │
                            └──────────────────────────────────────┘
```

### 10.2 The terminal contract

Any source that wants to feed the CLI must implement `iTerminalInterface` (defined in [src/utility/iIOInterface.h](src/utility/iIOInterface.h)). It extends `iIOInterface` with the byte-level I/O methods (`write`/`writeln` overloaded for every primitive, `readStringUntil`, `readLine`, `with_timestamp`) plus terminal-specific affordances. The framework already provides three implementations:

| Source | Class | Provided by |
|---|---|---|
| Serial port | `SerialInterface` (also satisfies `iSerialInterface : iClientInterface`) | Device port |
| Telnet client session | `iClientInterface*` returned by `iTcpServerInterface::accept()` | Device port (TCP) |
| SSH channel | LWSSH session wrapper | [src/service_provider/shell/ssh/](src/service_provider/shell/ssh/) |

When a telnet or SSH client connects, the corresponding service calls `__cmd_service.useTerminal(client)` to redirect the CLI's I/O to that session for the duration of the connection — then back to serial on disconnect.

### 10.3 Input sequences

The CLI does line editing in-process, so it has to recognise control sequences character-by-character. The enum is in [DataTypeDef.h](src/utility/DataTypeDef.h):

| Sequence | Action |
|---|---|
| `CMD_TERM_INSEQ_ENTER` | Submit current line for parsing/execution |
| `CMD_TERM_INSEQ_BACKSPACE_CHAR` / `_DELETE_CHAR` / `_DELETE` | Edit |
| `CMD_TERM_INSEQ_LEFT_ARROW` / `_RIGHT_ARROW` | Move cursor in-line |
| `CMD_TERM_INSEQ_UP_ARROW` / `_DOWN_ARROW` | Walk history (requires `ENABLE_STORAGE_SERVICE`) |
| `CMD_TERM_INSEQ_HOME` / `_END` | Line start / end |
| `CMD_TERM_INSEQ_PAGE_UP` / `_PAGE_DOWN` | Scroll long output |
| `CMD_TERM_INSEQ_TAB` | Autocomplete (cycles registered commands matching the typed prefix) |
| `CMD_TERM_INSEQ_ESC` | Cancel current line; for `fwrite`, finalize the file |
| `CMD_TERM_INSEQ_CTRL_C` / `_CTRL_Z` | Abort the running command; the base `executeTermInputAction` returns `CMD_RESULT_ABORTED` |

Long-running commands (`watch`, `fwrite`) opt-in to receive these sequences mid-execution by overriding `executeTermInputAction(cmd_term_inseq_t)`.

### 10.4 The `CommandBase` (`cmd_t`) contract

Every command is a `struct CommandBase` (typedef'd `cmd_t`). The struct owns parsing — you only fill in `execute(cmd_term_inseq_t)`. See [src/utility/CommandBase.h](src/utility/CommandBase.h).

#### Construction

A command class:
1. Calls `SetCommand("ls")` to declare its name (max `CMD_SIZE_MAX = 8` chars).
2. Calls `AddOption("p")`, `AddOption("m")`, … for each named option (max `CMD_OPTION_MAX = 3`, each name ≤ `CMD_OPTION_SIZE_MAX = 3` chars).
3. Optionally `setAcceptArgsOptions(true)` to also accept positional/free args.
4. Optionally `setCmdOptionSeparator(",")` (default), `";"`, or `" "`.
5. Implements `execute(cmd_term_inseq_t)` returning a `cmd_result_t`.
6. Optionally overrides `needauth()` to require login, `executeTermInputAction()` to handle CTRL+C/ESC/etc., `stopRunningInBackground()` for cleanup.
7. Registers itself via `CommandBase::RegisterCommand("ls", &ListFSCommand::Registrar)` so `__cmd_service` can dispatch by name.

#### Parsing rules

Given input `gpio p=4,m=3,v=500`:
- Tokenise the **command name** at the first space (or end).
- Look up the option separator (default `,`).
- For each `key=value` pair, find the key in the declared options and store `optionval` + `optionvalsize`.
- If `setAcceptArgsOptions(true)` and **no** `=` is present after the command, treat the rest as free positional args mapped into `m_options[0..]` in order.
- On success: `m_result = CMD_RESULT_OK`, then `execute()` runs.

Inside `execute`, options are read by name:

```cpp
auto pin = RetrieveOption(CMD_OPTION_NAME_P);
if (pin == nullptr) return CMD_RESULT_ARGS_MISSING;
int p = atoi(pin->optionval);
```

#### Multi-iteration commands

For commands that don't finish in a single tick (e.g. `watch`, `fwrite`), `execute` returns `CMD_RESULT_INCOMPLETE`. The service keeps the command active, increments `m_iterations`, and re-enters `execute` on every subsequent input. `Clear()` runs only when `m_result != CMD_RESULT_INCOMPLETE`.

#### Holding option values

By default, parsed `optionval` points into the live receive buffer — so once the next character arrives, the pointer is invalid. If a command needs the value to survive across iterations, it calls `holdOptionValue("c")` which copies the bytes into a `new char[]` owned by the option, freed on `Clear()`. `WatchCommand` uses this for its `c=<command>` payload that has to live for many ticks.

### 10.5 Result codes

| `cmd_result_t` | Meaning | What the dispatcher does |
|---|---|---|
| `CMD_RESULT_OK` | Success | Print blank line; clear options |
| `CMD_RESULT_INCOMPLETE` | Multi-iter; keep me active | Do not clear; wait for next input |
| `CMD_RESULT_ARGS_ERROR` / `_ARGS_MISSING` / `_INVALID_OPTION` | Bad usage | Print `CmdErr : <n>` |
| `CMD_RESULT_NOT_FOUND` / `_INVALID` | No such command / can't parse | Print error |
| `CMD_RESULT_NEED_AUTH` / `_WRONG_CREDENTIAL` | Login required / failed | Re-prompt via the login flow |
| `CMD_RESULT_ABORTED` | CTRL+C / CTRL+Z hit | Print error; stop iteration |
| `CMD_RESULT_FAILED` | Command logic failure | Print error |
| `CMD_RESULT_TERMINAL_ERR` / `_TERMINAL_ABORTED` / `_TERMINAL_HOLD_BUFFER` | Terminal-side states | Special-cased by the service |

### 10.6 `CommandLineServiceProvider`

[CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) — the runtime around `CommandBase`. Notable members:

| Member | Role |
|---|---|
| `m_cmdlist : vector<cmd_t*>` | Active command stack — usually one; multiple when `watch` runs a child command |
| `m_termrecvdata : pdiutil::string` | Per-keystroke receive buffer for the active line |
| `m_terminalCursorIndex : uint16_t` | In-line cursor position (for arrow keys) |
| `m_termhistoryfile` | Path to `/.term_history` (only with `ENABLE_STORAGE_SERVICE`) |
| `m_cmdHistoryIndex` | Walk position for ↑/↓ |
| `m_cmdAutoCompleteIndex` | Walk position for TAB |
| `processTerminalInput(t)` | Drain bytes from `t`, recognise sequences, feed `executeCommand` |
| `executeCommand(*cmd, inseq)` | Resolve name → `cmd_t` → parse → run |
| `useTerminal(t)` | Swap the bound terminal — used by telnet / SSH |
| `getCommandExecutedFromHistory(&out, idx, pattern=null)` | Reverse-walk the history file with optional substring filter |
| `startInteraction()` | Static; called from `PdiStack::initialize` to attach to serial and start the login prompt |

#### History & autocomplete

- **History** is persisted only when `ENABLE_STORAGE_SERVICE` is on. The file `CMD_TERMINAL_HISTORY_STATIC_FILEPATH = "/.term_history"` is capped at `CMD_TERMINAL_HISTORY_MAX_LINES = 25`. Older entries are rotated out.
- **Autocomplete** walks `CommandBase::m_cmd_registry` looking for names that start with the current input prefix. Subsequent TABs cycle through matches (`m_cmdAutoCompleteIndex`).
- Off devices without storage, ↑/↓ produce no output (no backing store) — autocomplete still works because the registry lives in RAM.

### 10.7 Built-in command inventory

Names come from [CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h); each `<Name>Command.h` defines the class. The full user-facing table with examples is in the main README's [Terminal section](#terminal) — this is just the source-to-command map.

| Command | Source | Service backing it |
|---|---|---|
| `login`, `logout` | [AuthCommand.h](src/service_provider/cmd/commands/AuthCommand.h) | `__auth_service` |
| `gpio` | [GpioCommand.h](src/service_provider/cmd/commands/GpioCommand.h) | `__gpio_service`, `__i_dvc_ctrl` |
| `srvc` | [ServiceCommand.h](src/service_provider/cmd/commands/ServiceCommand.h) | `ServiceProvider::getService` (any service) |
| `ls`, `cd`, `pwd`, `mkd`, `mkf`, `rm`, `mv`, `cp` | `ListFSCommand.h`, `ChangeDirFSCommand.h`, `PWDFSCommand.h`, `MakeDirFSCommand.h`, `MakeFileFSCommand.h`, `RemoveFSCommand.h`, `MoveFSCommand.h`, `CopyFSCommand.h` | `__i_fs` |
| `cat`, `fwrite` | [FileFSCommand.h](src/service_provider/cmd/commands/FileFSCommand.h) | `__i_fs` (`fwrite` is multi-iteration with ESC to finalize; `cat` was previously named `fread`) |
| `head`, `tail` | [HeadFSCommand.h](src/service_provider/cmd/commands/HeadFSCommand.h), [TailFSCommand.h](src/service_provider/cmd/commands/TailFSCommand.h) | `__i_fs` (`tail` uses `getOffsetFromLineNumber` for constant-memory backward read) |
| `wc`, `df` | [WcFSCommand.h](src/service_provider/cmd/commands/WcFSCommand.h), [DfFSCommand.h](src/service_provider/cmd/commands/DfFSCommand.h) | `__i_fs` |
| `grep` | [GrepFSCommand.h](src/service_provider/cmd/commands/GrepFSCommand.h) | `__i_fs` + [RegexMatch.h](src/utility/RegexMatch.h); recurses into directories |
| `hexdump` | [HexdumpCommand.h](src/service_provider/cmd/commands/HexdumpCommand.h) | `__i_fs` (16-byte streaming via `readFile`) |
| `cls` | [ClearScreenCommand.h](src/service_provider/cmd/commands/ClearScreenCommand.h) | Terminal |
| `scht` | [SchedulerTaskCommand.h](src/service_provider/cmd/commands/SchedulerTaskCommand.h) | `__task_scheduler` |
| `ssh` | [SSHCommand.h](src/service_provider/cmd/commands/SSHCommand.h) | `__sshserver_service` (key generation) |
| `tls` | [TlsCommand.h](src/service_provider/cmd/commands/TlsCommand.h) | `TlsCertProvisioner` (esp32 only, gated on `ENABLE_TLS_CERT_GENERATION`) — on-device cert/key generation with EC or RSA, IPv4 / DNS SANs |
| `reboot` | [RebootCommand.h](src/service_provider/cmd/commands/RebootCommand.h) | `__i_dvc_ctrl.restartDevice()` |
| `net` | [NetworkCommand.h](src/service_provider/cmd/commands/NetworkCommand.h) | `__wifi_service` |
| `watch` | [WatchCommand.h](src/service_provider/cmd/commands/WatchCommand.h) | `__task_scheduler` (multi-iter; CTRL+C aware) |
| `iot` | [DeviceIotCommand.h](src/service_provider/cmd/commands/DeviceIotCommand.h) | `__device_iot_service` |
| `help` | [HelpCommand.h](src/service_provider/cmd/commands/HelpCommand.h) | `CommandBase::m_cmd_registry` (works before login) |
| `uptime` | [UptimeCommand.h](src/service_provider/cmd/commands/UptimeCommand.h) | `__i_dvc_ctrl.millis_now()` (prints `up Xd Yh Zm Ws`) |

Most commands are 1-2 hundred lines; the only big one is `FileFSCommand` (311 lines) because `fwrite` carries a state machine for line-by-line entry, ESC handling, and append-mode file persistence. The newer FS commands (`head`, `tail`, `wc`, `grep`, `hexdump`) all use the **positional-arg** style (`setAcceptArgsOptions(true)` + space separator) rather than the `key=value` form — closer to the shell-tool idiom users expect.

### 10.8 Multi-terminal session lifecycle

```
boot
 │
 └─ CommandLineServiceProvider::startInteraction()
       useTerminal(__i_dvc_ctrl.getTerminal())     // serial
       prompt "login: …"
       ...

(later) telnet client connects on :23
 │
 └─ TelnetServiceProvider::handle()
       accept() → iClientInterface* c
       __cmd_service.useTerminal(c)                // CLI now writes to telnet
       loop until disconnect: processTerminalInput(c)
       on disconnect:
           __cmd_service.useTerminal(serialTerminal)   // back to serial

(later) SSH client connects on :22
 │
 └─ same pattern, additionally:
       authenticate, open channel, optionally SFTP subsystem
```

One CLI instance, three doorways. The implication: at most one user at a time has the prompt. A telnet session blocks the SSH and serial prompts until it disconnects.

### 10.9 SFTP / SCP file transfer

The SSH service ([§7.2.14](#7-service-providers)) opens an SFTP subsystem on demand. Two clients are supported: `scp -s …` (force SFTP) for single-file copies, and interactive `sftp` for full session browsing. Both ride the same handlers in `handleChannelSubsystemSftpRequest`. See the main README [SSH file transfer](#ssh-file-transfer) section for user-facing commands and the operation table.

Implementation notes:
- Bolus chunks (the SFTP protocol's data records) are received small (≤256 B per the SSH crypto window) and streamed straight into `__i_fs` — there is no full-file RAM buffer.
- File-edit overhead (overwriting an existing file) is significant on flash filesystems; the README's 0.2-1 kB/s figure reflects this.
- Only **one SFTP handle at a time** is tracked per session — opening a second file/dir while one is still open reuses the slot. Sufficient for `sftp` interactive use (one operation in flight) but not for parallel transfers.
- Directory listings are cached once at `OPENDIR` time into the session (`Sftp::dir_entries`) and paginated across multiple `READDIR` responses (16 entries per response). Memory is released on `CLOSE` or session destruction (RAII).
- SFTP sessions only have an **idle timeout** (default 60 s, in `handle()`); plain SSH shell sessions are not idle-reaped because a human may pause at the prompt indefinitely.

### 10.10 `watch` and CTRL+C semantics

`watch c=<command>; i=<ms>; n=<count>` is the framework's universal scheduler-on-demand. Internals:

1. Parses `c`, `i`, `n` (semicolon-separated because the inner command itself may use commas).
2. `holdOptionValue("c")` so the child command string survives across iterations.
3. Registers itself with `__task_scheduler.setInterval(... i ms ...)` — the periodic tick re-runs the child via `__cmd_service.executeCommand(&child, CMD_TERM_INSEQ_NONE)`.
4. Returns `CMD_RESULT_INCOMPLETE` so the service keeps it active.
5. On `CMD_TERM_INSEQ_CTRL_C`, the override of `executeTermInputAction` cancels the scheduler task, clears, and returns `CMD_RESULT_ABORTED`.

This is the model any user-supplied long-running command should copy.

### 10.11 Adding a new command

To add `temp` (read a temperature sensor):

1. **Decide options.** Say `t=<unit>` (C/F) — one option, default C.
2. **Add the name** to [CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h): `#define CMD_NAME_TEMP "temp"`.
3. **Create [src/service_provider/cmd/commands/TempCommand.h](src/service_provider/cmd/commands/TempCommand.h)**:
   ```cpp
   struct TempCommand : public CommandBase {
       TempCommand() {
           Clear();
           SetCommand(CMD_NAME_TEMP);
           AddOption(CMD_OPTION_NAME_T);
       }
       bool needauth() override { return true; }
       cmd_result_t execute(cmd_term_inseq_t) override {
           auto unit = RetrieveOption(CMD_OPTION_NAME_T);
           bool celsius = !(unit && unit->optionval[0] == 'F');
           float t = readSensor(celsius);
           m_terminal->writeln(t);
           return CMD_RESULT_OK;
       }
       static void* Registrar(void*) { static TempCommand cmd; return &cmd; }
   };
   ```
4. **Register it** by including the header in [CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) **and** calling `CommandBase::RegisterCommand(CMD_NAME_TEMP, &TempCommand::Registrar)` from `CommandLineServiceProvider::initService` (mirror the pattern used by sibling commands).
5. **Done.** `temp` will autocomplete via TAB, appear in history, and respect CTRL+C if you ever make it multi-iteration.

### 10.12 Gotchas

- **`CMD_SIZE_MAX = 8`** — command names are silently truncated past 8 chars. Use short verbs.
- **`CMD_OPTION_MAX = 3`** and **`CMD_OPTION_SIZE_MAX = 3`** — a command with 4+ options or option names longer than 2 chars (`+ null`) must split into sub-commands or use positional args.
- **One bound terminal at a time.** Telnet and SSH cannot both have the prompt simultaneously — the *first* connection wins until it disconnects. There is no session multiplexing.
- **History needs storage.** Without `ENABLE_STORAGE_SERVICE` the ↑/↓ keys are dead; the autocomplete via TAB still works (registry lives in RAM).
- **Option values are pointers into the input buffer by default.** If your `execute` may live across input arrivals (CTRL+C handler, multi-iteration), `holdOptionValue` first.
- **`needauth()` is the only gate.** A command returning `true` only fires after a successful `login`; setting it on a "harmless" command still adds friction. Use it for anything mutating state.
- **`watch` semicolon-separates options because the inner command can use commas.** When chaining your own composite commands, pick a separator that doesn't appear in payloads.
- **`fwrite` blocks the line until ESC.** Don't run it over telnet/SSH if you also rely on the watchdog firing inside the session — it does, but the terminal is unavailable for `srvc` / `scht` until you exit.
- **No quoted args.** Values cannot contain `,` (or whichever separator), `=`, or spaces inside an option value. Escape at the producer or accept the constraint.

---

## 11. Utility Library

[src/utility/](src/utility/) is the framework's foundation: small, dependency-light primitives every layer above uses. Anything in this directory may include the abstract interfaces from [src/interface/pdi/](src/interface/pdi/) but **not** any device header — the same `.cpp` must compile under every port without conditional includes.

Most of these have appeared in passing in earlier sections. This section is the index — what's in the box, the public API, and where each piece is used.

### 11.1 Inventory

| Component | Path | Purpose |
|---|---|---|
| **Interface foundations** | `iIOInterface.h`, `iUtilityInterface.h`, `iInstanceInterface.h` | Three abstract bases the entire framework rests on |
| **Type definitions** | `DataTypeDef.h` | `task_t`, `serial_event_t`, `CallBack*Fn` aliases, `cmd_term_inseq_t`, `ipaddress_t`, etc. |
| **Database engine** | `Database.{h,cpp}` | See [§6](#6-database-layer) |
| **Task scheduler** | `TaskScheduler.{h,cpp}` | See [§5](#5-task-scheduler) |
| **Command base** | `CommandBase.{h,cpp}` | See [§10](#10-command-line--terminal) |
| **Event bus** | `EventUtil.{h,cpp}` | Cross-service pub/sub |
| **String operations** | `StringOperations.{h,cpp}` | C-string helpers tuned for fixed-size NVM data |
| **Data type conversions** | `DataTypeConversions.{h,cpp}` | int/string/BCD/hex conversions without `printf` |
| **Base64** | `Base64.{h,cpp}` | Encoding + 16-byte unique key generator |
| **Regex match** | `RegexMatch.{h,cpp}` | Minimal regex engine — `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` `\<char>`. No alternation or grouping. Powers the `grep` CLI |
| **Safe alloc** | `SafeAlloc.{h,cpp}` | Heap-checked `pdiutil::safe_new<T>(args...)` / `safe_new_array<T>(n)` plus `safe_delete` / `safe_delete_array` (null-safe, ref-param sets pointer to `nullptr`). Refuses allocations that would breach `PDI_SAFE_ALLOC_HEAP_MARGIN` (default 2 KB headroom). Used by the TLS path to bail cleanly when handshakes can't safely allocate |
| **Queue / RingBuf / Proto** | `queue/queue.{h,cpp}`, `queue/ringbuf.{h,cpp}`, `queue/proto.{h,cpp}` | Byte queues and a length-prefixed parser |
| **Utility umbrella** | `Utility.{h,cpp}` | Single `#include <utility/Utility.h>` that pulls the lot in (conditional on `ENABLE_*`) |
| **Crypto** | `crypto/` | SHA-1/256/512, HMAC-SHA1, AES (ECB/CBC/CTR), Curve25519, Ed25519 |
| **PdiSTL** | `pdistl/` | A trimmed C++ standard library subset for memory-constrained devices |
| **Fiber** | `fiber/` | Reserved namespace, currently empty |

### 11.2 Interface foundations (recap)

Documented in [§3.1](#3-portable-interfaces); included here only for cross-reference.

- **`iIOInterface` / `iTerminalInterface`** — `write`/`writeln` family overloaded for every primitive type, padding helpers (`write_pad`), VT100 escape helpers (`with_timestamp`, `clear`, `home`, `cursor_pos`), `readStringUntil` / `readLine` with optional yield callback.
- **`iUtilityInterface`** — `wait(timeoutms)`, `millis_now()`, `yield()`, `log(type, msg)`, optional stack measurement.
- **`iInstanceInterface`** — factory for fresh TCP server/client instances and the file-system singleton.

### 11.3 `EventUtil` — cross-service pub/sub

[EventUtil.h](src/utility/EventUtil.h). Single global `__utl_event` that owns a vector of `(event_name_t, CallBackVoidPointerArgFn)` listeners. Used everywhere a service needs to react to another service's state change without a direct dependency — see [§7.4](#7-service-providers).

| Method | Purpose |
|---|---|
| `begin(iUtilityInterface*)` | Bind the time source; called once from `PDIStack` ctor ([PdiStack.cpp](src/PdiStack.cpp)) |
| `add_event_listener(event, handler)` | Subscribe. Returns `false` if the listener table is full |
| `execute_event(event, arg=nullptr)` | Synchronously fire every listener for `event` |
| `m_last_event` | Public for the rare case a listener wants to know which event triggered it |

`event_name_t` lives in [EventConfig.h](src/config/EventConfig.h) — `EVENT_FACTORY_RESET`, `EVENT_WIFI_*`, `EVENT_INTERNET_*`, `EVENT_GPIO_TRIGGER`, `EVENT_OTA_*`. Adding a new event is just adding to the enum.

### 11.4 `StringOperations` — fixed-size C-string toolkit

[StringOperations.h](src/utility/StringOperations.h). Designed for the **fixed-size, no-`String`-class** style the NVM config structs use ([§4.4](#4-configuration-system)). Every function takes an explicit length bound (default 300) — they are safe against missing null terminators.

| Function | One-line purpose |
|---|---|
| `__strstr(str, substr, len=300)` | Find substring; returns index or -1 |
| `__strtrim(str, _overflow_limit=300)` | Trim leading/trailing whitespace in place; returns pointer past leading spaces |
| `__strtrim_val(str, val, limit=300)` | Trim a specific char from both ends |
| `__are_str_equals(s1, s2, limit=300)` | Null-terminated comparison up to limit |
| `__are_arrays_equal(a1, a2, len=300)` | Raw byte compare |
| `__appendUintToBuff(str, format, value, len)` | `printf`-free integer append |
| `__int_ip_to_str(str, ip[], len=15)` | IPv4 bytes → dotted-quad |
| `__str_ip_to_int(str, ip[], len=15, clear_after=true)` | Dotted-quad → IPv4 bytes |
| `__find_and_replace(str, find, replace, occurrence)` | In-place replace (Nth occurrence) |
| `__get_from_json(str, key, value, max_value_len)` | Tiny JSON value extractor (no full parser) |
| `__tolowercase(str, strlen)` / `__touppercase(str, strlen)` | In-place case conversion |
| `__get_iface_key_informat(iface, port, &upper, &lower, max)` | Build interface-key strings for GPIO/serial JSON payloads |
| `__get_iface_data_fromjson(iface, port, json, len, &out, max)` | Pull a per-interface payload from a JSON envelope |

These are the only string utilities in the framework — there is no `pdiutil::string::trim()` etc. equivalent. Use these for any work over the NVM-shaped char arrays.

### 11.5 `DataTypeConversions` — no-`stdio` numeric formatting

[DataTypeConversions.h](src/utility/DataTypeConversions.h). Why a custom set: `printf` family pulls in formatter code that costs 4-8 KB of flash on AVR/ESP8266 builds.

| Function | Purpose |
|---|---|
| `BcdToUint8(val)` / `Uint8ToBcd(val)` | RTC byte conversions |
| `StringToUint64/32/16/8(str, len=32)` | Bounded numeric parse |
| `StringToHex16(str, strlen)` | Hex string → `uint16_t` |
| `Int32ToString(val, pString, maxlen, padmax=0)` | Integer → string with optional zero-padding |
| `Int64ToString(val, pString, maxlen, padmax=0)` | Same, 64-bit |
| `Int32DigitCount(x)` / `Int64DigitCount(x)` | Number of decimal digits |

The `scht` command uses these for its column-aligned task table ([TaskScheduler.cpp](src/utility/TaskScheduler.cpp)).

### 11.6 `Base64`

[Base64.h](src/utility/Base64.h). Two free functions:

- `bool base64Encode(input[], len, *res)` — standard Base64; caller sizes `res` to `4 * ceil(len / 3) + 1`.
- `void genUniqueKey(char *key, int len)` — fills `key` with a pseudo-random Base64-alphabet token. Used by `EwSessionHandler::create_session` for cookie tokens.

There is no decode helper. Anyone that needs to decode (SMTP `AUTH LOGIN` response, SSH parsing) writes inline against the alphabet.

### 11.7 Queues — `QUEUE`, `RINGBUF`, `PROTO`

Three layers, originally from `tuanpmt/esp_mqtt`, retained intact:

| Layer | Path | Use |
|---|---|---|
| **`RINGBUF`** | [queue/ringbuf.h](src/utility/queue/ringbuf.h) | Byte ring with `RINGBUF_Init(r, buf, size)`, `RINGBUF_Put(r, c)`, `RINGBUF_Get(r, *c)` — the substrate |
| **`QUEUE`** | [queue/queue.h](src/utility/queue/queue.h) | Length-tagged record queue built on a ring; `QUEUE_Init`, `QUEUE_Puts`, `QUEUE_Gets`, `QUEUE_IsEmpty` |
| **`PROTO_PARSER`** | [queue/proto.h](src/utility/queue/proto.h) | Length-prefixed packet parser with a completion callback — used by MQTT to assemble frames out of TCP byte streams |

These are used internally by [src/transports/mqtt/](src/transports/mqtt/) and incidentally by the serial event bus. New code should reach for `pdiutil::vector` or `pdistl::deque` unless the byte-level discipline of the ring matters.

### 11.8 Crypto — `src/utility/crypto/`

The framework ships a minimal but production-quality crypto kit. Everything is plain C-style functions over fixed-size buffers; no allocation, no state hidden in globals (except per-context structs you own).

#### Hashes

| Algorithm | Header | API shape |
|---|---|---|
| SHA-1 | [hash/sha1.h](src/utility/crypto/hash/sha1.h) | `SHA1Init/Update/Final`, one-shot `SHA1(out, in, len)` |
| SHA-256 | [hash/sha256.h](src/utility/crypto/hash/sha256.h) | `sha256_init/update/final`, one-shot `sha256(msg, len, out)` |
| SHA-512 | [hash/sha512.h](src/utility/crypto/hash/sha512.h) | `sha512_init/update/final`, one-shot `sha512(msg, len, out)` |
| HMAC-SHA1 | [hmac/hmac_sha1.h](src/utility/crypto/hmac/hmac_sha1.h) | One-shot `hmac_sha1(key, klen, data, dlen, out)` |

#### Symmetric

| Cipher | Header | Modes |
|---|---|---|
| AES-128 / AES-256 (compile-time) | [symmetric/aes/aes.h](src/utility/crypto/symmetric/aes/aes.h) | `ECB_encrypt/decrypt(ctx, buf)`, `CBC_encrypt/decrypt_buffer(ctx, buf, len)`, `CTR_xcrypt_buffer(ctx, buf, len[, resetCtx])` |

`struct AES_ctx` is initialised with `AES_init_ctx(ctx, key)` or `AES_init_ctx_iv(ctx, key, iv)`; reset the IV mid-stream via `AES_ctx_set_iv`.

#### Asymmetric

| Algorithm | Header | API highlights |
|---|---|---|
| Curve25519 (X25519 ECDH) | [asymmetric/curve25519/curve25519.h](src/utility/crypto/asymmetric/curve25519/curve25519.h) | `crypto_scalarmult_base(q, n)`, `crypto_scalarmult(q, n, p)`, `curve25519_create_keypair(pub, priv)`, `curve25519_create_keypair_with_ed25519privkey(pub, priv, ed_priv)` |
| Ed25519 (signing) | [asymmetric/ed25519/ed25519.h](src/utility/crypto/asymmetric/ed25519/ed25519.h) | `ed25519_create_seed`, `ed25519_create_keypair(pub, priv, seed)`, `ed25519_sign(sig, msg, len, pub, priv)`, `ed25519_verify(sig, msg, len, pub) → int`, `ed25519_key_exchange(shared, pub, priv)`, `ed25519_private_to_curve25519` for SSH bridging |

All asymmetric primitives are upstream from the standard portable Ed25519 reference (see [ed25519/readme.md](src/utility/crypto/asymmetric/ed25519/readme.md)). They are used by [src/service_provider/shell/ssh/](src/service_provider/shell/ssh/) for SSH host key generation, the key exchange handshake, and signing the SSH banner. AES-CTR powers the SSH transport encryption.

#### `fixedint.h`

[crypto/fixedint.h](src/utility/crypto/fixedint.h) is a small stdint shim used by the crypto primitives — only relevant if you port them to a non-C99 toolchain.

### 11.9 PdiSTL — `src/utility/pdistl/`

A subset of the C++ standard library trimmed for embedded targets. Adopted from [mike-matera/ArduinoSTL](https://github.com/mike-matera/ArduinoSTL) (which is itself based on uClibc++). Lives under namespace `pdiutil` and `pdistd` (use `pdiutil::` for containers, `pdistd::` for `min/max/swap`).

| Header | What's there |
|---|---|
| `string` | `pdiutil::string` — the framework's std::string substitute. Used pervasively |
| `vector` | `pdiutil::vector<T>` — backs the task list, service registry, controllers, DB tables |
| `functional` | `pdiutil::function<...>` — backs `CallBackVoidArgFn` |
| `algorithm`, `numeric` | `pdistd::min/max/swap`, etc. |
| `array`, `deque`, `list`, `map`, `set`, `queue`, `stack`, `bitset` | Containers — present, less commonly used |
| `memory` | `pdiutil::shared_ptr` / `unique_ptr` shims |
| `iostream`, `ostream`, `istream`, `streambuf`, `sstream` | Stream classes — usable, but the framework prefers `iTerminalInterface` |
| `type_traits`, `iterator`, `utility`, `initializer_list` | Type machinery |
| `cmath`, `cstdint`, `cstdlib`, `cstring`, `ctime`, `cassert`, `climits`, `cfloat`, `cstdio`, `cwchar`, `cwctype`, `csignal`, `csetjmp`, `cstdarg`, `cstddef`, `cinttypes`, `clocale`, `cerrno`, `cctype` | C library wrappers |
| `complex`, `valarray`, `locale`, `stdexcept`, `typeinfo`, `exception` | Less-used corners |
| `abi/` | C++ ABI glue (`abi.cpp`, `eh_alloc`, `eh_globals`) — provides `__cxa_*` for toolchains that lack it |
| `system_configuration.h` | Per-feature toggles (`__UCLIBCXX_*`) — enable/disable optional parts at compile time |

The `.old` files (`new_*.cpp.old`, `del_*.cpp.old`) are legacy placement-new operators kept for reference; the active implementations are in `support.cpp`.

**Rule of thumb:** in framework code, use `pdiutil::string` and `pdiutil::vector`; use the namespace prefix explicitly (no `using namespace`) so a future swap to the host `std::` is mechanical.

### 11.10 The umbrella header

[Utility.h](src/utility/Utility.h) is the **one include** that pulls in: the right scheduler variant (timer vs. cooperative based on `ENABLE_TIMER_TASK_SCHEDULER`), event bus, conversions, string ops, queue, Base64, I/O, and `CommandBase` (when CMD is enabled). Anything in `src/` that does `#include <utility/Utility.h>` gets the whole foundation in one shot. New code should follow this pattern rather than reaching for individual files.

### 11.11 Gotchas

- **`fiber/` is empty.** It's a reserved namespace for future coroutine/fiber primitives. Don't try to import from it.
- **`pdiutil::string` is not `std::string`.** Some methods are missing or renamed; check the header before relying on a particular API. SBO size and allocator semantics differ from libstdc++.
- **`StringOperations` always takes a length.** Don't drop the default — the framework's NVM strings are often non-null-terminated up to their buffer size, and the default-300 cap is what keeps the helpers safe.
- **Crypto routines are constant-time only where the upstream made them so.** Ed25519 verify is, AES is table-based and timing-side-channel sensitive. Don't run on a network where timing attacks are in the threat model without auditing.
- **`Base64` has no decode.** Plan for that if your code path needs round-trip Base64.
- **`PROTO_PARSER` keeps state across calls.** Don't share one parser across two connections; allocate one per stream.
- **The umbrella header has a transitive cost.** It pulls 20+ headers. If a file *only* needs `StringOperations`, include that one directly to keep your TU lean — `Utility.h` is for files that genuinely consume the whole surface.

---

## 12. Logger

The logger is intentionally the smallest subsystem in the framework. There is **no log buffer, no rotating sink, no per-tag filtering, no JSON formatter** — just four levels, a printf-style varargs helper, and an interface that ports usually implement with one line per method. The cleverness is in *how it disappears*: when no log level is enabled, every `LogI(...)` / `LogE(...)` / `LogFmtI(...)` site in the codebase compiles to nothing — zero flash, zero RAM, zero runtime cost.

Implementation: [src/interface/pdi/iLoggerInterface.h](src/interface/pdi/iLoggerInterface.h) (port contract), [src/utility/DataTypeDef.h](src/utility/DataTypeDef.h) (default no-op macros), `devices/<board>/LoggerInterface.{h,cpp}` (port-side overrides).

### 12.1 Levels

| `logger_type_t` | Macro | Format-macro | When to use |
|---|---|---|---|
| `INFO_LOG` | `LogI(msg)` | `LogFmtI(fmt, args...)` | State transitions, lifecycle ("`Starting MQTT Service`") |
| `ERROR_LOG` | `LogE(msg)` | `LogFmtE(fmt, args...)` | Recoverable failures, validation rejects |
| `WARNING_LOG` | `LogW(msg)` | `LogFmtW(fmt, args...)` | Unusual conditions, deprecated paths |
| `SUCCESS_LOG` | `LogS(msg)` | `LogFmtS(fmt, args...)` | Confirmation of high-value operations (`"OTA applied"`) |

Plus one lifecycle macro:

- **`LOGBEGIN`** — expands to `__i_logger.init()`, called once from [`PDIStack::initialize`](src/PdiStack.cpp) to set up the logger sink (typically `Serial.begin(115200)`).

### 12.2 The four feature flags

| Flag | Effect |
|---|---|
| `ENABLE_LOG_INFO` | Enables `LogI` / `LogFmtI` |
| `ENABLE_LOG_ERROR` | Enables `LogE` / `LogFmtE` |
| `ENABLE_LOG_WARNING` | Enables `LogW` / `LogFmtW` |
| `ENABLE_LOG_SUCCESS` | Enables `LogS` / `LogFmtS` |
| `ENABLE_LOG_ALL` | Shorthand for *all of the above* |

All five are commented out by default in [devices/DeviceConfig.h](devices/DeviceConfig.h). The shipping configuration is **silent** — release builds emit nothing on Serial.

### 12.3 The disappearing-macro pattern

The trick that makes the logger zero-cost when off is a two-layer macro definition. **Layer 1** lives in [DataTypeDef.h](src/utility/DataTypeDef.h) and is the unconditional stub:

```cpp
#define LOGBEGIN
#define LogI(v)              // info log
#define LogE(v)              // error log
#define LogW(v)              // warning log
#define LogS(v)              // success log
#define LogFmtI(f, args...)
#define LogFmtE(f, args...)
#define LogFmtW(f, args...)
#define LogFmtS(f, args...)
```

Every translation unit that includes any utility header sees these — so `LogI("...")` is always legal C++, even on a build with no logger. It just expands to nothing.

**Layer 2** lives in the *device's* `LoggerInterface.h` (e.g. [devices/esp32/LoggerInterface.h](devices/esp32/LoggerInterface.h)) and runs only if (a) that header is actually included and (b) the matching `ENABLE_LOG_*` is on:

```cpp
#if defined(LogI) && (defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ALL))
#undef LogI
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
#endif
```

Whether the device's `LoggerInterface.h` gets included is itself gated in the per-board aggregator ([devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)):

```cpp
#if defined(LOGBEGIN) && \
    (defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || \
     defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || \
     defined(ENABLE_LOG_SUCCESS))
#include "LoggerInterface.h"
#endif
```

So the cascade is:

```
No ENABLE_LOG_* set
    ↓
devices/<board>/LoggerInterface.h is NOT included
    ↓
Layer-2 #undef/#define never runs
    ↓
Macros stay as the empty stubs from DataTypeDef.h
    ↓
Every Log* call site compiles to nothing
    ↓
__i_logger symbol is never referenced → linker drops it
```

Net effect: a *zero-byte* logger when disabled, identical to manually `#ifdef`-stripping every call site — but written naturally.

### 12.4 The port contract — `iLoggerInterface`

Six pure-virtual methods + one initialiser:

| Method | Signature | Notes |
|---|---|---|
| `init()` | `void init()` | One-time setup; called via `LOGBEGIN` |
| `log(type, content)` | `void log(logger_type_t, const char*)` | Generic entry; dispatches by type |
| `log_info(s)` / `log_error(s)` / `log_warning(s)` / `log_success(s)` | `void log_*(const char*)` | Per-level fast paths |
| `log_format(fmt, type, ...)` | `void log_format(const char*, logger_type_t, ...)` | Varargs printf-style |

Singleton: `extern LoggerInterface __i_logger;`. The base class is abstract; the per-device subclass typically just forwards each method to `Serial.print` (see [LoggerInterface.cpp](devices/esp32/LoggerInterface.cpp)).

### 12.5 The flash-string convention

Every macro wraps the argument in `RODT_ATTR(...)`:

```cpp
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
```

That means the literal string lives in flash memory (`PROGMEM` on AVR, `.rodata` on ESP), and the logger reads through it as a flash pointer. On AVR/ESP8266 this is what keeps a verbose `LogI("...")` from chewing through RAM — a few hundred log call sites cost flash space they were going to occupy anyway, and zero RAM.

The implementation side has to honour this: `log_info(const char* s)` may be receiving a *flash pointer*, not a RAM pointer. The default device impl uses `Serial.print` which handles both transparently on Arduino cores. A custom port that writes through a non-Arduino API must call the flash-aware variant (`strlen_P`, `memcpy_P`, the `rofn::to_charptr` helper in [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp)).

### 12.6 Where logs go

The default sink is the device's serial port at 115200 baud — `LoggerInterface::init()` calls `Serial.begin(115200)`. There is **no** built-in routing to:

- Telnet / SSH sessions (those have their own terminal lanes via `iTerminalInterface`)
- The file system (no rotating log file)
- The web portal (the dashboard doesn't show log lines)
- A remote syslog / HTTP sink

If you need any of those, write a custom `LoggerInterface` for your port whose methods fan out to multiple sinks (Serial + your destination). The interface contract supports it; the framework just doesn't ship the wiring.

### 12.7 Typical usage in framework code

Three patterns recur across services:

```cpp
LogBegin (in PdiStack::initialize)              // boot the logger sink

LogI("Starting WiFi service");                  // simple level
LogE("Found invalid configs.. starting factory reset!");

LogFmtI("NTP Validity : %d\n", __i_ntp.is_valid_ntptime());   // formatted
LogFmtE("OTA failed for %s -> %d", url, status);
```

There is **no** convention for module tags (`"[wifi] starting"` etc.) — services prepend their name in-line if they want one. Keep messages short; long strings cost flash.

### 12.8 Enabling logs on a running build

To turn logs on:

1. Uncomment one or more `ENABLE_LOG_*` in [devices/DeviceConfig.h](devices/DeviceConfig.h):
   ```c
   #define ENABLE_LOG_INFO
   #define ENABLE_LOG_ERROR
   // or just: #define ENABLE_LOG_ALL
   ```
2. Recompile and flash. Open Serial Monitor at 115200.

There is no runtime toggle. To selectively log only one service, the practical option is to gate that service's calls behind a custom macro (`#define LogMqtt(v) LogI(v)` and only include it in MQTT files), then leave only `ENABLE_LOG_INFO` on. The framework doesn't ship per-tag filters.

### 12.9 Gotchas

- **`LOGBEGIN` must be called or `Serial` is never started.** [`PDIStack::initialize`](src/PdiStack.cpp) does this on your behalf — don't `Serial.begin()` again in your sketch's `setup()` or you'll race the logger.
- **Macros are space-sensitive.** `LogFmtI("%d\n", x);` works; `LogFmtI ("%d\n", x);` (space after the name) doesn't — `args...` is a GCC variadic-macro extension and is brittle to whitespace on some toolchains.
- **`Log*("constant string")` works; `Log*(my_var)` may not.** `RODT_ATTR(v)` only does the right thing when `v` is a string literal. If you want to log a `const char*` variable, use `LogFmtI("%s", my_var)` — the format string is a literal, the variable is just a `%s` argument.
- **No level-filtering at runtime.** Disabling `ENABLE_LOG_INFO` removes every `LogI` from the binary; you cannot turn one back on without recompiling.
- **`SerialServiceProvider` and the logger share the same serial port by default.** On boards with only one UART, this is fine because both call `Serial.print`. On boards where you've remapped the CLI to a second UART, the logger still goes to `Serial` — override `LoggerInterface::log_*` to route there too if you want them unified.
- **No timestamping.** Lines arrive as-written, no prefix. If you want timestamps, prepend them with `LogFmtI("[%lu] ...", millis())` — there's no global setting.
- **Format-macro varargs use GCC's `args...` syntax.** Won't compile under MSVC / older non-GNU compilers. Embedded toolchains (avr-gcc, xtensa-gcc) all handle it; off-device testing with MSVC won't.

---

## 13. Memory & Performance Notes

This section is a cross-cutting reference of the **memory model** and **what each subsystem actually costs** in flash, RAM, and CPU. None of it introduces new mechanics — it collects the constraints that have shown up in pieces through earlier sections so you can size a build before you compile it.

### 13.1 The memory budget per target

Rough usable budget after the Arduino core, lwIP, and stdlib are linked. Treat these as orders of magnitude, not exact ceilings.

| Target | Flash | RAM | NVM (EEPROM-emulated) | Realistic ceiling |
|---|---|---|---|---|
| Arduino UNO (ATmega328P) | 32 KB | 2 KB | 1 KB | Serial + Storage(EEPROM) + GPIO_BASIC + CMD; **no network** |
| ESP8266 | 1 MB | ~50 KB free heap | ~4 KB | Default full build minus SSH (key sizes + crypto blow heap); contextual execution feasible |
| ESP32 | 4 MB | ~250 KB free heap | ~4 KB | Default full build comfortably; can spare a second contextual lane |

These are why [DeviceConfig.h](devices/DeviceConfig.h) gates entire feature groups on `DEVICE_ARDUINOUNO` and why `MAX_DB_TABLES` is 5 on UNO vs. 15 on esp\*.

### 13.2 Where every byte lives

| Region | What's there | Set by |
|---|---|---|
| `.text` (flash) | Code + string literals declared `PROG_RODT_ATTR` + `RODT_ATTR("...")` literals | The compiler; you influence it via flash-string macros |
| `.rodata` | True const tables (precomputed Ed25519 / AES tables, address constants) | Compiler |
| `.data` / `.bss` (RAM) | Global singletons — `__i_dvc_ctrl`, `__i_db`, every `__<x>_service`, `__task_scheduler`, `__database`, `__utl_event`, `__web_route_handler`, the `Controller::m_controllers` vector | Static-init |
| Heap | `pdiutil::vector` / `pdiutil::string` storage, `m_client = __i_instance.getNewT(cp\|ls)ClientInstance()` ([PdiStack.cpp](src/PdiStack.cpp)), per-session SSH and TLS buffers (BearSSL/mbedTLS I/O records sized by `TLS_IBUF_SIZE` / `TLS_OBUF_SIZE`), controller HTML scratch | `new` / `pdiutil` containers + `pdiutil::safe_new` (heap-checked) |
| NVM | All `*_config_table` structs (§4) framed with +5 bytes overhead each | `iDatabaseInterface` |
| Per-task stacks (contextual only) | One per `TASK_MODE_COOPERATIVE` / `TASK_MODE_PREEMPTIVE` task | `scheduleUnderExecSched(..., stackdepth)` |

The framework's discipline is to push as much as possible into flash and as little as possible into heap. Sizing problems almost always show up as **heap fragmentation after hours of uptime**, not as boot-time overflow — see §13.7.

### 13.3 Flash strings — `RODT_ATTR` and `PROG_RODT_ATTR`

Two macros, one purpose: keep string literals out of RAM.

| Macro | Defined where | Effect on devices with flash strings (esp*) | Effect on others (mock, UNO) |
|---|---|---|---|
| `RODT_ATTR("text")` | `devices/<board>/<board>_device_config.h` | Expands to `(const char*)F("text")` — wraps the literal in the Arduino `F()` macro so it stays in flash | Falls through to the default `#define RODT_ATTR(x) x` in [DataTypeDef.h](src/utility/DataTypeDef.h) — plain literal, identical behaviour |
| `PROG_RODT_ATTR` | same | Expands to `PROGMEM` (storage qualifier on the variable) | Empty — variable lives in normal `.rodata` |
| `PROG_RODT_PTR` | same | Expands to `PGM_P` — the right pointer type to read flash strings on AVR | Empty — `const char*` works directly |

**Rule:** every string literal that ships in the binary belongs in `RODT_ATTR(...)` (when used inline) or in a `static const char foo[] PROG_RODT_ATTR = "...";` (when held in a named variable). The framework's CLI prompts, terminal output, HTTP page fragments, log messages, and every `srvc` name follow this rule — it's why the binary is dense rather than RAM-hungry.

### 13.4 Reading a flash string back — `rofn::to_charptr`

Some consumers can't accept a flash pointer directly (e.g. a function that calls `strcpy` instead of `strcpy_P`). The framework exposes `rofn::to_charptr(const void* rostr)` ([DataTypeDef.h](src/utility/DataTypeDef.h); implementation per device, e.g. [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp)) that:

1. Reads the flash string into a fresh `new char[len+1]`.
2. Returns the RAM pointer.
3. **Caller owns the memory** — must `delete[]` it.

Use this sparingly. Every call allocates from the heap; it's a one-way escape hatch, not a default.

### 13.5 What each feature actually costs

Approximate incremental cost on **ESP8266** (the binding constraint for most users) when you enable a flag on top of the minimum build. Numbers from inspection, not measurement — within a factor of two but not a benchmark.

| Flag | Flash | RAM idle | Heap per session | Notes |
|---|---|---|---|---|
| Baseline (`SERIAL`+`STORAGE`+`CMD`+DB+factory reset) | ~110 KB | ~6 KB | — | Logger / WiFi off |
| `+ ENABLE_WIFI_SERVICE` | ~+60 KB | ~+8 KB | — | Includes scan buffers |
| `+ ENABLE_NETWORK_SERVICE` (NTP, ping, TCP) | ~+15 KB | ~+2 KB | — | |
| `+ ENABLE_MQTT_SERVICE` | ~+30 KB | ~+1 KB | +1-2 KB per pub burst | Includes 3 config tables |
| `+ ENABLE_OTA_SERVICE` | ~+10 KB | small | +6 KB during download | The upgrade path also reserves flash region |
| `+ ENABLE_EMAIL_SERVICE` | ~+15 KB | small | +1 KB per send | SMTPClient + Base64 of credentials |
| `+ ENABLE_HTTP_SERVER` | ~+50 KB | ~+3 KB | +PAGE_HTML_MAX_SIZE (1.8 KB) per request | The 12 controllers + page fragments |
| `+ ENABLE_GPIO_SERVICE` | ~+10 KB | ~+0.5 KB | small | Plus event-channel HTTP-post buffer if used |
| `+ ENABLE_TELNET_SERVICE` | ~+5 KB | small | +2 KB per session | One session at a time |
| `+ ENABLE_SSH_SERVICE` | **~+150 KB** | ~+4 KB | **+8-16 KB per session** | Ed25519 + Curve25519 + AES tables + per-session key state |
| `+ ENABLE_DEVICE_IOT` | ~+10 KB | ~+1 KB | small | Mostly config |
| `+ ENABLE_NAPT` | small flash | **+10-20 KB heap** | n/a | lwIP NAPT table; the biggest hidden cost on a "small" build. Mutually exclusive with `ENABLE_TLS_SERVICE` on esp8266 |
| `+ ENABLE_TLS_SERVICE` (esp8266 / BearSSL) | ~+120 KB | ~+2 KB | **+10-15 KB per active session** — `(TLS_IBUF_SIZE + TLS_OBUF_SIZE)` I/O buffers (~3 KB) + BearSSL engine + cert chain state (~3-5 KB) + the `TLS_TASK_STACK_SIZE` (6.5 KB) cooperative task | Implicitly forces `ENABLE_CONTEXTUAL_EXECUTION` on. Default `TLS_IBUF_SIZE = 2048` is below BearSSL's spec maximum (~17 KB) — peers that emit 16 KB records will fail the handshake; bump only if needed |
| `+ ENABLE_TLS_SERVICE` (esp32 / mbedTLS) | ~+50 KB (mbedTLS already linked by WiFi/lwIP) | ~+4 KB | **+35-50 KB per active session** — `mbedtls_ssl_context` + `ssl_config` + x509 chain + pk context + entropy/RNG ≈ 10 KB + record buffers `MBEDTLS_SSL_IN/OUT_CONTENT_LEN` ≈ 32 KB at IDF defaults + our `TLS_IBUF_SIZE` rxQueue (~2 KB) + worker FreeRTOS task stack (`TLS_TASK_STACK_SIZE`/sizeof(StackType_t) ≈ 6.5 KB) | Wrapper code only adds ~5-8 KB flash; mbedTLS itself is already in flash for WiFi/lwIP. Record buffers dominate the per-session cost — trim `MBEDTLS_SSL_IN/OUT_CONTENT_LEN` in `sdkconfig.defaults` to ~4-8 KB if you can guarantee no large server certs. esp32's hardware AES/SHA/RSA acceleration is used transparently |
| `+ ENABLE_HTTPS_SERVER` | small flash on top of `TLS_SERVICE` | small | adds one TLS session per concurrent client | Reuses `iTlsServerInterface` from the TLS port |
| `+ ENABLE_TLS_CERT_GENERATION` (esp32 only) | ~+15 KB (mbedTLS x509write/PK-write code) | small | **+keygen transient ~3-5 KB** on cert mint + ~200-500 ms ECDSA-P256 keygen with HW accel; PEM buffers freed after `writeFile` | Requires `MBEDTLS_X509_CREATE_C`, `MBEDTLS_X509_CRT_WRITE_C`, `MBEDTLS_PK_WRITE_C`, `MBEDTLS_PEM_WRITE_C` in IDF mbedTLS config (esp32-arduino default has all four). Gates both the boot-time `EVENT_WIFI_STA_GOT_IP` auto-provisioner and the `tls` CLI command |
| `+ ENABLE_CONTEXTUAL_EXECUTION` | ~+10 KB | small | +`stackdepth` bytes per cooperative/preemptive task | Plus timer ISR for preemption. Available on both esp8266 and esp32 now |
| Any `ENABLE_LOG_*` | a few hundred bytes | nil | n/a | The format-string footprints live in flash |

The numbers above are *additive* but several features share parts of the same buffer pool (any service that needs a TCP client reuses `m_client` in `PdiStack`).

### 13.6 The expensive features, called out

A small number of choices dominate the budget:

- **SSH** is by far the heaviest enable. If you turn it on, you've effectively committed to esp32-class memory.
- **TLS** is comparable to SSH in flash and worse than NAPT in heap — and `ENABLE_TLS_SERVICE` **cannot coexist with `ENABLE_NAPT` on esp8266** because both want too much of the same fixed heap. Pick one. Inbound HTTPS (`ENABLE_HTTPS_SERVER`) and outbound TLS clients share the same per-port BearSSL/mbedTLS code so there's no extra cost for enabling both.
  - **Per-live-session footprint matters more than the flat enable cost.** Each accepted (or outbound) TLS client triggers, *for the lifetime of that one connection*: a dedicated worker task (one stack — `TLS_TASK_STACK_SIZE` ≈ 6.5 KB, allocated from heap on FreeRTOS), the TLS I/O record buffers, plus SSL workflow state (session keys, cipher contexts, certificate chain in RAM during validation, handshake scratch). On **esp8266 / BearSSL** this drops free heap by **10-15 KB per active session** — drastic on a ~30-40 KB working budget; `TLS_IBUF_SIZE + TLS_OBUF_SIZE` (~3 KB) dominates, BearSSL engine state adds another 3-5 KB. On **esp32 / mbedTLS** the drop is **~35-50 KB per active session** — most of it is mbedTLS's internal `IN_CONTENT_LEN` + `OUT_CONTENT_LEN` record buffers (default 16 KB each in IDF), plus ~10 KB of `ssl_context`/`config`/x509/pk/RNG state. Trim `MBEDTLS_SSL_IN/OUT_CONTENT_LEN` in `sdkconfig.defaults` to drop esp32 per-session below 20 KB if you control your peer's record sizes. **All of that memory is released as soon as the client closes** — the worker task exits (FreeRTOS reaps the stack via the idle task on esp32, so reclamation may lag the close by one main-loop yield), buffers are freed, session state is torn down. Plan capacity for the worst-case number of concurrent TLS clients, not the average; an idle build looks healthy because nothing is allocated.
- **NAPT** is invisible in flash but expensive in heap because lwIP allocates the translation table. Disable on builds that don't bridge the AP to the STA.
- **HTTP server controllers** each carry their own form-validation code; the 12 of them add up. If you don't need the portal, drop `ENABLE_HTTP_SERVER` even when `ENABLE_WIFI_SERVICE` stays on.
- **Contextual execution** is cheap in flash but the per-task stacks live in RAM for the lifetime of the task. Two cooperative tasks at 1 KB each is 2 KB of RAM you can't reclaim — and turning on `ENABLE_TLS_SERVICE` adds at least one 6.5 KB cooperative task on esp8266 to host BearSSL work off the cont_t stack.
- **`pdiutil::string` and `pdiutil::vector` allocate** — even though they hide the heap, every push/append can fragment over time. Reserve up-front (`m_tasks.reserve(MAX_SCHEDULABLE_TASKS)` in [TaskScheduler.cpp](src/utility/TaskScheduler.cpp)) when the size is known.

### 13.7 Heap discipline

The framework expects to run for **weeks to months** without rebooting. The patterns that keep it stable:

1. **Don't `new` after `setup()`.** The single `m_client = __i_instance.getNewT(cp|ls)ClientInstance()` in [PdiStack.cpp](src/PdiStack.cpp) is acceptable because it happens once at static-init (the TLS branch picks the right factory at compile time via `ENABLE_TLS_SERVICE`). Anything that allocates per request, per tick, or per event will fragment.
2. **Reserve container sizes up front.** Both `__task_scheduler.m_tasks` and `__database.m_database_tables` reserve at `init` time so push_backs don't reallocate.
3. **Reuse buffers.** Web controllers reuse the `WebResourceProvider`'s 1.8 KB scratch. Transports reuse caller-owned `iClientInterface*` instances. The MQTT parser uses a static `PROTO_PARSER` ring.
4. **Hold-then-free, never hold-forever.** `CommandBase::holdOptionValue` ([§10.4](#10-command-line--terminal)) allocates a fresh buffer for the option value, but `Clear()` frees it at the end of every command iteration.
5. **No std-exception machinery.** PdiSTL's exception support is stubbed where the toolchain allows; `throw`/`catch` outside crypto/`pdistl` corners costs more than it's worth on a 50 KB heap.
6. **Check the heap before allocating big.** For code paths that genuinely *can* be skipped under memory pressure (TLS handshake, large web response composition), prefer `pdiutil::safe_new<T>(args...)` / `safe_new_array<T>(n)` from [SafeAlloc.h](src/utility/SafeAlloc.h) — they refuse the allocation (returning `nullptr`) if the free heap would dip below `PDI_SAFE_ALLOC_HEAP_MARGIN` (default 2 KB), letting the caller bail cleanly instead of crashing inside a third-party lib.

When something fragments, the symptom is `register_task` returning `-1`, `new` returning `nullptr` inside a transport, or `pdiutil::vector` failing to grow. The framework doesn't `LogE` these by default — wire your own check on `__task_scheduler.m_tasks.size()` if you suspect a leak.

### 13.8 CPU and tick budget

The main loop is `PdiStack::serve()` ([§1.6](#1-architecture-overview)). Every iteration:

| Step | Typical cost |
|---|---|
| `__web_server.handle_clients()` | <1 ms idle, 5-50 ms during a request |
| `__task_scheduler.run()` | <1 ms idle; runs at most **one** inline task per iteration (the "run-one-then-break" pattern, [§5.5](#5-task-scheduler)) |
| `__i_dvc_ctrl.yield()` | Vendor SDK time slice — 1-3 ms typical |
| `__i_dvc_ctrl.handleEvents()` | <1 ms unless a service handler is slow |
| Contextual scheduler ticks (if enabled) | Depends on per-task work |

Loop frequency ranges from **hundreds of Hz idle** down to **~10 Hz during heavy HTTP/SSH activity**. Two implications:

- **Anything that needs sub-100 ms response should not live in an inline task.** Use the `contextual` lanes or accept the jitter.
- **WiFi yields are not free.** Reducing `WIFI_CONNECTIVITY_CHECK_DURATION` below 5 s starts to compete with serve-loop work.

### 13.9 Boot-time profile

```
power-on
 ├─ static-init: every __<x> global constructed     ~10-50 ms
 ├─ PDIStack ctor: m_client = __i_instance.getNewT(cp|ls)ClientInstance() ~1 ms
setup()
 ├─ __database_service.initService                  ~50-200 ms  (NVM read of every table)
 ├─ serial init, terminal greeting                  ~10 ms
 ├─ wifi init + STA scan                            ~500-3000 ms  (the dominant boot cost)
 ├─ http server, telnet, ssh accept loops           ~50 ms
 ├─ cmd_service prompt                              ~10 ms
loop() — steady state begins
```

Total to first prompt: ~1-4 seconds depending on whether STA connection happens immediately.

### 13.10 Performance-sensitive choices

- **`RODT_ATTR` everywhere.** Skipping it doesn't break anything but it does silently move 50-200 bytes of strings to RAM per file. Always wrap.
- **`pdiutil::string` vs. `char[]`.** Use `char[]` for NVM-shaped fixed-size data (the config structs); use `pdiutil::string` for transient runtime work where size isn't known. Mixing them in one struct defeats the NVM serialisation contract.
- **`int` vs. `int32_t` in config tables.** Always use the explicit-size type — `sizeof(int)` differs between AVR and ESP and that breaks NVM portability between ports of the same build (though the framework doesn't ship NVM images across ports today).
- **Avoid `printf` family.** Use [DataTypeConversions](src/utility/DataTypeConversions.h) — the framework saves 4-8 KB by not pulling in libc's formatter.
- **SFTP throughput cap.** SCP/SFTP is 0.2-1 KB/s ([README §SSH file transfer](#ssh-file-transfer)). Large file movement is not a use case; firmware-update should go through OTA, not SFTP.

### 13.11 Profiling on-device

Three handles for runtime visibility:

- **`scht`** lists every scheduler task with its last `_task_exec_millis` — your best signal that a service is hogging the loop ([§5.9](#5-task-scheduler)).
- **`srvc s=<n>,q=2`** prints each service's status — covers DB validity, WiFi connectivity, MQTT connect state.
- **`iUtilityInterface::measure_lastfn_stack()`** ([§3.3.1](#3-portable-interfaces)) — optional; a port that implements it lets you wrap critical work with `__i_dvc_ctrl.measure_lastfn_stack()` calls to estimate per-fn stack high-water marks. esp* ports don't implement it today.

### 13.12 Gotchas

- **Global singletons all construct before `main()`.** Don't put device-touching code in any ctor (already covered in [§3.5](#3-portable-interfaces), [§7.6](#7-service-providers)) — also don't let a ctor allocate, because the heap may not be ready on some toolchains.
- **`pdiutil::vector::reserve` is a *hint*.** Pushing past the reservation reallocates and copies. Always reserve to the *worst case*, not the *typical case*.
- **`PAGE_HTML_MAX_SIZE = 1800`** is per-`send` chunk, not per-response. Compose pages in three calls ([§9.7](#9-web-server)) — one big string overflows silently.
- **`NAPT` table growth is unbounded** by default in lwIP. If your AP carries dozens of clients, the heap will climb. The framework has no per-client throttle.
- **History file writes happen per-keystroke** during `fwrite` ([§10.9](#10-command-line--terminal)). For long files, the flash wear is real — `fwrite` is for small config edits, not log capture.
- **`__task_scheduler` runs *one* inline task per `serve()` tick.** Twenty registered tasks at 100 ms cadence each can starve each other if your serve loop runs less than 10 times per second. Profile with `scht`.

---

## 14. Build & Toolchain

PDI Framework targets the **Arduino IDE / arduino-cli** build system. This is the supported path, and it dictates almost every structural quirk you've read about so far — most notably the `.cpp`-include trick ([§2.4](#2-device-layer--porting-guide)) and the codegen step ([§6.5](#6-database-layer)). This section is the integrator's reference: install, board versions, what the scripts do, what `library.properties` declares, and the open issues that keep the library off the Arduino Library Manager.

### 14.1 Supported boards

| Board | Arduino IDE board package version |
|---|---|
| Arduino UNO | 1.8.6 |
| ESP8266 | 3.1.2 |
| ESP32 | 3.3.3 |

These are the versions the current `devices/<board>/` adapters are written against — vendor SDK headers and Arduino-core APIs move, so newer board packages may break a port until the adapter is updated.

### 14.2 `library.properties`

```
name=pdi-framework
version=1.1.8
author=Suraj I.
maintainer=Suraj I. <surajinamdar151@gmail.com>
architectures=avr,esp8266,esp32
dot_a_linkage=true
```

Two values worth understanding:

- **`architectures=avr,esp8266,esp32`** — the IDE will only let you compile the library when the active board belongs to one of these three architectures. A new port (`stm32`, `nrf52`, …) needs an entry here.
- **`dot_a_linkage=true`** — tells `arduino-cli` to first compile the library to a static archive (`pdi-framework.a`) before linking it into the sketch. Combined with the `--gc-sections` and `-ffunction-sections` flags every Arduino core sets by default, this is what makes the linker drop entire services when their `ENABLE_*` flag is off — `library.a` contains every translation unit, but the linker keeps only the ones the sketch (transitively via `PdiStack`) actually references.

### 14.3 Installation flow

End-user steps (the "happy path"):

1. **Install the device's Arduino board package** at the version above through the Boards Manager.
2. **Clone the framework** into the device's `libraries/` directory:
   ```
   cd ~/Arduino/libraries
   git clone https://github.com/Suraj151/pdi-framework.git
   cd pdi-framework
   ```
   Linux/macOS: `~/Arduino/libraries/`. Windows: `%USERPROFILE%\AppData\Local\Arduino15\packages\<vendor>\hardware\<arch>\<ver>\libraries\` (cross-arch install) **or** `Documents\Arduino\libraries\` (per-user).
3. **Pull submodules:**
   ```
   git submodule update --init --recursive
   ```
   This fetches LittleFS into [external/littlefs/](external/littlefs/) — required for `ENABLE_STORAGE_SERVICE` and everything that depends on it (SSH, SFTP, history).
4. **Run the setup script** for the chosen device:
   ```
   cd scripts
   python3 DeviceSetup.py -d esp8266        # or esp32 / arduinouno
   ```
   This writes [devices/DeviceSetup.h](devices/DeviceSetup.h) and regenerates [src/database/tables/](src/database/tables/).
5. **Open the bundled example** in the IDE: File → Examples → pdi-framework → PdiStack. Select the matching board, compile, flash.

That's the whole install. Step 4 is the one that prevents Library Manager distribution — see §14.7.

### 14.4 What the scripts do

| Script | Purpose | When to re-run |
|---|---|---|
| [`scripts/DeviceSetup.py`](scripts/DeviceSetup.py) | Writes `devices/DeviceSetup.h` with `#define DEVICE_<NAME>` ([§2.5](#2-device-layer--porting-guide)) and then calls `CreateDBSourceFromJson.py` | When switching target device |
| [`scripts/CreateDBSourceFromJson.py`](scripts/CreateDBSourceFromJson.py) | Wipes [src/database/tables/](src/database/tables/) and regenerates it from the per-device `DBTableSchema.json` by calling `JsonToCpp.py` ([§6.5](#6-database-layer)) | When changing the schema for the active device |
| [`scripts/JsonToCpp.py`](scripts/JsonToCpp.py) | Generic JSON-to-C++ codegen used by the table generator | Always indirectly |
| [`scripts/Util.py`](scripts/Util.py) | Shared helpers: template substitution, kebab-to-PascalCase, `clang-format` invocation, device-folder path resolution | Library — not invoked directly |
| [`scripts/GenTlsCerts.py`](scripts/GenTlsCerts.py) | OpenSSL-backed off-device TLS cert generator — EC or RSA keypair, multi-DNS / multi-IP SANs, optional CA mode, signing CSRs against a stored CA. Output to `certs/`, then upload via SFTP to the device FS at the `TLS_DEFAULT_*_PATH` defaults. The on-device equivalent for esp32 is the `tls` CLI command. | When provisioning HTTPS on esp8266 (no on-device gen), or seeding an esp32 cert that needs to be signed by a stable dev CA |

Generated output goes through `clang-format --style=Microsoft -i` if `clang-format` is on `PATH`, so generated headers look hand-written. If the formatter is missing, files are written un-formatted and still compile fine.

### 14.5 What's git-ignored

[.gitignore](.gitignore):

```
/src/database/tables           # entirely autogenerated by CreateDBSourceFromJson
/devices/DeviceSetup.h         # autogenerated by DeviceSetup
/scripts/__pycache__/
.vscode/
```

This is why a fresh clone has no `DeviceSetup.h` and an empty `database/tables` — the setup script is mandatory before the first build.

### 14.6 Submodules

[.gitmodules](.gitmodules) declares one:

```
[submodule "external/littlefs"]
    path = external/littlefs
    url = https://github.com/littlefs-project/littlefs.git
```

LittleFS is the on-device filesystem the storage interface wraps for esp8266/esp32. Without it pulled in, any `ENABLE_STORAGE_SERVICE` build fails at link time with missing `lfs_*` symbols. AVR builds skip storage entirely so the submodule isn't strictly required there.

The [lwip/](lwip/) folder is **not** a submodule — it's a vendored copy of customised lwIP 1.4 for the legacy NAPT path documented in the main README's [Features → NAT](#features) section. The modern path uses the lwIP shipped with the ESP8266 core; this folder is opt-in by manually replacing the board-package's lwIP.

### 14.6 The Arduino "src-only" rule and why it matters

The Arduino IDE / `arduino-cli` build only compiles `.cpp` / `.c` files that live **directly inside `<library>/src/`** (subdirectories of `src/` are recursed; siblings of `src/` are not). This is what forces the [§2.4](#2-device-layer--porting-guide) trick:

- Per-device `.cpp` files under [devices/<board>/](devices/) would be invisible to the builder if they weren't pulled into a `src/`-resident compilation unit.
- The bridge: `src/interface/pdi.h` is included from `src/PdiStack.cpp` (which *is* in `src/`); `pdi.h` picks `<device>_pdi.h`; that header is paired with `<device>_pdi.cpp` (also under `devices/<board>/`) which is reached through the same `pdi.h`-aggregator dance via a sibling `.cpp` that lives in `src/` and `#include`s it.

The practical consequence: every device translation unit ends up flattened into a **single big object file** per build. That's why `static` symbols inside `devices/<board>/*.cpp` are file-scope-of-the-aggregator, not file-scope-of-the-source. Don't reference `devices/<board>/*.cpp` from outside that chain — it would link twice.

### 14.7 Why the Arduino Library Manager isn't supported

Three blockers, in priority order:

1. **The codegen step.** Library Manager publishes a static zip of the repo; it can't run `python3 DeviceSetup.py -d <name>` before the user's first build. `devices/DeviceSetup.h` and `src/database/tables/*.h` would be missing. Fix paths: ship pre-generated headers for all supported boards plus auto-select via `#if defined(ESP32)` / `#if defined(ESP8266)` / `#if defined(__AVR_ATmega328P__)`; **or** move the table generation to a `constexpr` table-descriptor pattern at compile time.
2. **The git submodule.** Library Manager fetches a tarball, not a `git clone --recursive`. `external/littlefs/` would be empty on Library Manager installs. Fix path: vendor the LittleFS sources into `src/` directly under a license-compatible drop, or switch to a single-file LittleFS port.
3. **The `.cpp`-include layout.** Even with #1 and #2 fixed, the device adapters live outside `src/` and rely on the aggregator trick. The Library Manager catalogue tolerates this — but only because `src/`-resident files transitively include the device `.cpp`s. Any future Arduino tooling that enforces "no `#include` of `.cpp`" would break us. Fix path: move `devices/<board>/` into `src/devices/<board>/` and delete the aggregator includes — at the cost of slightly less obvious project layout.

None of these are conceptual blockers, just engineering work. The README's Installation section calls this out: "***installation from Arduino library manager for this library is not supported due to autogen scripts which required to generate the device specific config and DB table sources***."

### 14.8 Defines the build relies on

A handful of macros must reach every translation unit; the toolchain provides them:

| Macro | Source | Required for |
|---|---|---|
| `DEVICE_ESP32` / `DEVICE_ESP8266` / `DEVICE_ARDUINOUNO` | `devices/DeviceSetup.h` (autogen) | Device-aggregator selection in `src/interface/pdi.h` |
| `ENABLE_*` | `devices/DeviceConfig.h` | Service / interface gating (§4) |
| `ARDUINO` | Arduino core | Whether to use Arduino APIs at all |
| `ESP32` / `ESP8266` | Arduino core | Used by some interface impls cross-checking the port macro vs. the board macro |
| `__AVR_*` | avr-gcc | UNO-specific code paths |
| `PROGMEM`, `PGM_P`, `strlen_P`, `memcpy_P` | Arduino core | Flash-string helpers wrapped by `RODT_ATTR` / `rofn::to_charptr` (§13.3) |

If you build outside Arduino IDE (PlatformIO, raw `make`), reproduce these defines and you'll be fine — the framework has no hidden compiler-flag dependencies beyond standard `-std=c++14` (or newer) with GCC variadic-macro extensions enabled.

### 14.9 Editor support

[.vscode/settings.json](.vscode/settings.json) ships only language associations so the PdiSTL headers (`vector`, `algorithm`, `basic_definitions`, …) are recognised as C++. No build tasks. If you want IntelliSense for the active device, add a `compile_commands.json` via `arduino-cli compile --output-dir ...` or use the Arduino IDE 2.0's built-in C++ language server.

### 14.10 Reproducible builds and CI

There is no CI configuration in the repo today. A minimal CI matrix would be:

```
matrix:
  device: [arduinouno, esp8266, esp32]
  flags:  [minimal, full]
steps:
  - git submodule update --init --recursive
  - python3 scripts/DeviceSetup.py -d ${{ matrix.device }}
  - sed flag edits into devices/DeviceConfig.h for "minimal"/"full"
  - arduino-cli compile --fqbn <fqbn for device> examples/PdiStack
```

The framework is deterministic per `(device × flag-set)` — the codegen produces byte-identical output, and the `library.properties` `architectures` list bounds the matrix.

### 14.11 Gotchas

- **Forgetting `git submodule update --init --recursive`** is the #1 build error reported by new contributors. The link error names `lfs_*` symbols; the fix is always the submodule.
- **Forgetting the setup script** produces a missing-`DEVICE_*` error that surfaces as the mock device being selected, which then misses any header `ENABLE_NETWORK_SERVICE` references. Re-run `DeviceSetup.py -d <board>`.
- **Switching boards without re-running setup.** `devices/DeviceSetup.h` keeps the *previous* device macro. Always re-run the script when changing target.
- **Library installed from a tarball.** Some users download the GitHub "Code → Download ZIP". This skips submodules. Either clone with git, or download LittleFS separately and drop it under `external/littlefs/`.
- **`dot_a_linkage=true` masks weak-symbol bugs.** If two TUs define the same global non-`weak` symbol, the linker error is friendly with object-file linkage and obscure with archive linkage. When you see "multiple definition" only on a release build, it's usually from `.a` linkage hiding a one-instance-per-device singleton violation.
- **`clang-format` is optional but recommended.** Without it, generated tables look ugly but compile; with it, they match the rest of the repo.
- **The IDE bundles its own arduino-cli.** If you `pip install` or `brew install` a separate `arduino-cli` for scripting, the version mismatch can produce different binary output between the IDE and CI. Pin both.

---

## 15. Examples Walkthrough

[examples/](examples/) ships two tracks: one **end-user** example that just brings the whole framework up, and one **`Dev/` tree** with task-focused snippets demonstrating each extension surface. They are minimum-viable code on purpose — copy, adapt, ship.

Available examples:

| Example | Path | Lines | Demonstrates |
|---|---|---|---|
| `PdiStack` | [examples/PdiStack/PdiStack.ino](examples/PdiStack/PdiStack.ino) | 13 | The smallest possible sketch — `initialize()` + `serve()` |
| `TaskScheduling` | [examples/Dev/TaskScheduling/TaskScheduling.ino](examples/Dev/TaskScheduling/TaskScheduling.ino) | 112 | All three task modes (inline / cooperative / preemptive) plus update + clear |
| `AddingDatabaseTable` | [examples/Dev/AddingDatabaseTable/AddingDatabaseTable.ino](examples/Dev/AddingDatabaseTable/AddingDatabaseTable.ino) | 123 | Ad-hoc app-side persistence without going through codegen |
| `AddingController` | [examples/Dev/AddingController/AddingController.ino](examples/Dev/AddingController/AddingController.ino) | 93 | Adding a custom web route with auth middleware |
| `MqttExample` | [examples/Dev/MqttExample/MqttExample.ino](examples/Dev/MqttExample/MqttExample.ino) | 120 | Programmatic MQTT config + publish / subscribe callbacks |
| `DeviceIotExample` | [examples/Dev/DeviceIotExample/](examples/Dev/DeviceIotExample/) | 184 | Implementing `iDeviceIotInterface` to feed the IoT pipeline |

### 15.1 `PdiStack` — the everything-on sketch

```cpp
#include <PdiStack.h>

void setup() {
    PdiStack.initialize();
}

void loop() {
    PdiStack.serve();
}
```

That's the whole file. Every service in [devices/DeviceConfig.h](devices/DeviceConfig.h) that is `ENABLE_*`'d brings itself up under [`PDIStack::initialize`](src/PdiStack.cpp), and `serve()` ticks them all on every loop iteration. The sketch you flash to a brand-new device.

What you should see after flash:

1. Serial @ 115200 prints the boot banner (`Starting PDIStack ! / Release / Config`).
2. WiFi AP `pdiStack` (password `pdiStack@123`) appears in the available networks.
3. Connect, browse to `http://192.168.0.1`, log in as `pdiStack` / `pdiStack@123`.
4. The dashboard, GPIO, MQTT, OTA, Email, Storage menus are all live.

**When to start here:** every other example assumes `initialize()` ran first. Use this to validate the build before adding code.

### 15.2 `TaskScheduling` — the scheduler in three modes

This example layers four behaviours on the bare stack:

- A one-shot `timeout_task` fired 1 s after boot via `__task_scheduler.setTimeout(...)`.
- A periodic `interval_task` started at 3 s cadence via `setInterval`, with the returned id captured in `variable_time_task_id`.
- A second one-shot at 15 s that calls `updateInterval` to **change the periodic task's cadence** to 1 s.
- A third one-shot at 30 s that calls `clearInterval` to remove it.

Inside `#ifdef ENABLE_CONTEXTUAL_EXECUTION` (esp8266 and esp32 both ship the threading port now), two more tasks demonstrate the contextual lanes — each is a `while(1) { … sleep(500); }` body promoted via `scheduleUnderExecSched` with a 1 KB per-task stack:

```cpp
int cooperative_task_id = __task_scheduler.register_task(cooperative_task);
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler, cooperative_task_id,
    TASK_MODE_COOPERATIVE, 1 * 1024);
```

Maps directly to [§5. Task Scheduler](#5-task-scheduler) — particularly §5.4 (API by use case) and §5.8 (contextual scheduling).

The comment at line 76-77 is worth heeding: **disable `ENABLE_LOG_*`** before running, or the framework's own log output will interleave with the demo prints and obscure the schedule.

### 15.3 `AddingDatabaseTable` — app-side persistence without codegen

The framework's normal DB flow runs through `DBTableSchema.json` and the codegen ([§6.5](#6-database-layer)). This example shows the **escape hatch**: define a `DatabaseTable<addr, T>` subclass directly in your sketch and register it via the same static-init mechanism the framework uses for its own tables.

Key lines:

```cpp
#if defined(DEVICE_ARDUINOUNO)
#define STUDENT_TABLE_ADDRESS  800
#else
#define STUDENT_TABLE_ADDRESS  2500     // addresses ≤ 2499 reserved by framework
#endif

struct student_table { student_t students[MAX_STUDENTS]; int student_count; };

class StudentTable : public DatabaseTable<STUDENT_TABLE_ADDRESS, student_table> {};
StudentTable __student_table;          // static-init self-registers (see §6.3)
```

Then in `setup` after `PdiStack.initialize()`: build a `student_table` value, call `__student_table.set(&value)`. A scheduled task every 5 s reads back via `__student_table.get(&value)` and prints it.

Three things this example teaches well:

1. **Address picking.** The comment at lines 9-14 is the canonical guidance — pick `≥ 2500` on esp\* (`≥ 800` on UNO) to stay out of the framework's pre-allocated range from [DBTableSchema.json](devices/esp32/config/DBTableSchema.json).
2. **POD struct discipline.** `student_t` is a fixed-size `_name[20]` + `uint8_t` + `enum` — no `String`, no pointer, no `pdiutil::string`. Required by the raw-NVM serialiser ([§4.4](#4-configuration-system), [§6.7](#6-database-layer)).
3. **No JSON schema edit needed.** Because the table lives in the sketch, the codegen never sees it. Trade-off: it won't appear in `srvc` listings or any other reflective tooling that walks the generated tables.

### 15.4 `AddingController` — extending the web portal

A new route `/test-route` behind `AUTH_MIDDLEWARE`. The pattern from [§9.4](#9-web-server) and [§9.10](#9-web-server) made concrete:

```cpp
class TestController : public Controller {
public:
    TestController() : Controller("test") {}

    void boot() override {
        m_route_handler->register_route(
            "/test-route",
            [&]{ this->handleTestRoute(); },
            AUTH_MIDDLEWARE);
    }

    void handleTestRoute() {
        char* _page = new char[PAGE_HTML_MAX_SIZE];
        memset(_page, 0, PAGE_HTML_MAX_SIZE);
        strcat_ro(_page, WEB_SERVER_HEADER_HTML);
        strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP);
        concat_svg_menu_card(_page, WEB_SERVER_HOME_MENU_TITLE_DASHBOARD,
                             SVG_ICON48_PATH_DASHBOARD, WEB_SERVER_DASHBOARD_ROUTE);
        strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM);
        strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
        m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
        delete[] _page;
    }
};

TestController test_controller;        // static-init self-registers (see §9.4)
```

Things to copy:

- **Self-registration via the constructor** — no manual list to update.
- **`strcat_ro(...)` for flash-string concatenation** — the framework's flash-aware version of `strcat`, preserving the `RODT_ATTR` discipline of [§13.3](#13-memory--performance-notes).
- **`AUTH_MIDDLEWARE`** is the only line that gates the route; without it, the route is public.

Things to **not** copy verbatim:

- The 5 KB `new char[PAGE_HTML_MAX_SIZE]` allocation per request. Real controllers (see [§9.7](#9-web-server)) compose pages in chunked `send(..., true)` calls so the response streams to the client without an intermediate buffer. The example's allocator-based version is illustrative; chunked is the right pattern for production.
- The hard-coded comment "PAGE_HTML_MAX_SIZE defined in framework as 5000" is **stale** — it's currently 1800. See [§9.7](#9-web-server).

### 15.5 `MqttExample` — programmatic MQTT setup

This is the pattern for any sketch that wants MQTT configured **from code**, not from the web portal — useful for fleet provisioning. Three sequential steps in `configure_mqtt()`:

1. **Read** the three MQTT tables from NVM via `__database_service.get_mqtt_*_config_table(&local)`.
2. **Overwrite** the relevant fields with `memcpy(local.host, MQTT_HOST, …)` etc.
3. **Write** the tables back with `__database_service.set_mqtt_*_config_table(&local)`.
4. **Register callbacks** with `__mqtt_service.setMqttPublishDataCallback(...)` and `setMqttSubscribeDataCallback(...)`.
5. **Trigger the service to reload** via `__task_scheduler.setTimeout([&]{ __mqtt_service.handleMqttConfigChange(); }, 10, millis())`.

The 10 ms `setTimeout` is doing real work — it defers the reload off the `setup()` stack so any pending log output flushes first. The pattern recurs across services (WiFi, OTA, IoT).

The callback signatures match [§7.2.7](#7-service-providers):
- `void publish_callback(char* payload, uint16_t length)` — fills `payload` with the data to publish.
- `void subscribe_callback(uint32_t* args, const char* topic, uint32_t tlen, const char* data, uint32_t dlen)` — called per inbound message.

The `[mac]` token in `MQTT_CLIENT_ID`, topics, and the LWT message is replaced by the framework with the device's MAC at runtime — handy for fleet uniqueness without sketch-side templating.

### 15.6 `DeviceIotExample` — implementing `iDeviceIotInterface`

The IoT service is the only one whose contract requires the **application** to implement an interface ([§3.3.5](#3-portable-interfaces)). This example has the implementation split across two files:

- [DeviceIotSensor.h](examples/Dev/DeviceIotExample/DeviceIotSensor.h) — declares `class DeviceIotSensor : public iDeviceIotInterface` and the per-sensor state (`m_sensor_samples[]`, `m_sensor_sample_index`, `m_sensor_sample_value`).
- [DeviceIotSensor.cpp](examples/Dev/DeviceIotExample/DeviceIotSensor.cpp) — implements `init()`, `sampleHook()` (gather one sample), `dataHook(pdiutil::string& payload)` (build the JSON to publish), `resetSampleHook()` (clear the sample buffer).
- [DeviceIotExample.ino](examples/Dev/DeviceIotExample/DeviceIotExample.ino) — registers the sensor:
  ```cpp
  DeviceIotSensor sensor;
  void setup() {
      PdiStack.initialize();
      __device_iot_service.initDeviceIotSensor(&sensor);
  }
  ```

Once `__device_iot_service.initDeviceIotSensor(&sensor)` is called, the service drives the schedule itself: `sampleHook` runs at the configured sample rate, `dataHook` runs at the configured publish rate, and the payload is pushed to the configured MQTT channel ([§7.2.9](#7-service-providers)).

This is the **only** example with a sketch-side `.h` and `.cpp` rather than a single `.ino` — necessary because `iDeviceIotInterface` has a constructor + multiple virtual overrides, which Arduino's `.ino` preprocessor handles poorly for non-trivial classes.

### 15.7 Suggested example order for a new contributor

1. **`PdiStack`** — confirm the toolchain works end-to-end.
2. **`TaskScheduling`** — the scheduler is the most useful primitive once you start adding behaviour.
3. **`MqttExample`** — see the get-set-callback-trigger pattern in real code; reusable for WiFi/OTA/IoT.
4. **`AddingDatabaseTable`** — if your project has app-specific persistent state.
5. **`AddingController`** — if your project needs a custom web page.
6. **`DeviceIotExample`** — only if you're publishing telemetry through the IoT pipeline.

### 15.8 Gotchas

- **Examples assume `DeviceSetup.py` has already run.** Without it, every example fails to compile with mock-device misses ([§14.7](#14-build--toolchain)).
- **Most examples `#error` if their dependency flag is off.** `AddingController` requires `ENABLE_HTTP_SERVER`; `MqttExample` requires `ENABLE_MQTT_SERVICE`; `DeviceIotExample` requires `ENABLE_DEVICE_IOT`. The default `DeviceConfig.h` has all three on, but if you've trimmed for memory, re-enable before running the matching example.
- **`AddingController` references a stale `PAGE_HTML_MAX_SIZE = 5000`.** Current value is 1800 ([§9.7](#9-web-server)). The example's response will fit, but the *comment* should not be quoted as authoritative.
- **`TaskScheduling`'s contextual section needs `ENABLE_CONTEXTUAL_EXECUTION`.** Off by default even on esp8266. Uncomment the matching block in [devices/DeviceConfig.h](devices/DeviceConfig.h).
- **`AddingDatabaseTable` writes to NVM on every boot.** Repeated flash + boot cycles wear the storage. Comment out the writes once you've verified the read path.
- **Multiple examples cannot be flashed at once.** The IDE compiles one `.ino` per build — you cannot have `PdiStack`'s `loop()` *and* `TaskScheduling`'s task registrations together by opening both files. Copy the bits you want into a single sketch.

---

## 16. Extending the Framework

Every section so far has its own "how do I add one of these?" subsection. This section is the **cross-reference index** — pick what you're building, follow the cookbook, jump to the deep dive when you need it. The intent is that an experienced embedded developer can skim this page and ship a non-trivial extension without re-reading the whole document.

### 16.1 Decision: which extension surface fits?

| You want to… | Reach for | Section |
|---|---|---|
| Support a new MCU / board | **Add a device port** | [§16.2](#162-add-a-new-device) → [§2](#2-device-layer--porting-guide) |
| Add a missing hardware capability across all ports | **Add a portable interface** | [§16.3](#163-add-a-new-interface) → [§3.6](#3-portable-interfaces) |
| Add a new framework-level feature (logging sink, fleet manager, …) | **Add a service provider** | [§16.4](#164-add-a-new-service-provider) → [§7.5](#7-service-providers) |
| Speak a new wire protocol (CoAP, gRPC, …) | **Add a transport** | [§16.5](#165-add-a-new-transport) → [§8.7](#8-transports) |
| Persist new configuration to NVM | **Add a database table** | [§16.6](#166-add-a-new-database-table) → [§6.9](#6-database-layer) |
| Add a screen to the on-device portal | **Add a web page / controller** | [§16.7](#167-add-a-new-web-page) → [§9.10](#9-web-server) |
| Add a terminal command | **Add a CLI command** | [§16.8](#168-add-a-new-cli-command) → [§10.11](#10-command-line--terminal) |
| Hand-roll persistence in a sketch without touching codegen | **Use the DB escape hatch** | [§15.3 `AddingDatabaseTable`](#153-addingdatabasetable--app-side-persistence-without-codegen) |
| React to a service event without coupling to it | **Subscribe to the event bus** | [§16.9](#169-react-to-an-event) → [§7.4](#7-service-providers), [§11.3](#11-utility-library) |
| Run periodic / long-running work | **Use the task scheduler** | [§5](#5-task-scheduler) |
| Encrypt outbound HTTP / MQTT / SMTP / OTA without changing service code | **Turn on `ENABLE_TLS_SERVICE`** — orchestrator swaps `m_client` to a TLS-capable instance, every service that holds it auto-upgrades | [§7.2.16](#7-service-providers), [§13.5](#135-what-each-feature-actually-costs) |
| Serve the on-device portal over HTTPS | **Turn on `ENABLE_HTTPS_SERVER`** + drop cert/key into `/etc/http/server.{crt,key}` | [§7.2.12](#7-service-providers), [§7.2.16](#7-service-providers) |
| Provision an HTTPS cert | **`tls` CLI** on esp32 *or* [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) off-device | [§14.4](#14-build--toolchain), [§17.5](#175-frequently-asked-questions) |
| Talk to an existing service from the sketch | **Call the singleton** | [§16.10](#1610-call-an-existing-service-from-a-sketch) |

If your need straddles two of these, the **lower** layer in the stack is usually the right answer — adding a transport is preferable to adding a service that hard-codes a protocol; adding an interface is preferable to a service that touches the vendor SDK.

### 16.2 Add a new device

Full guide: [§2.9](#2-device-layer--porting-guide). The shape:

1. `devices/<board>/` folder with the umbrella header (`<board>.h` for SDK includes), the platform-macro header (`<board>_device_config.h` for `RODT_ATTR` / `strcat_ro` / `CRITICAL_SECTION_*`), and the two aggregators (`<board>_pdi.h` / `<board>_pdi.cpp`).
2. Implement the **always-required** trio (`DeviceControlInterface`, `DatabaseInterface`, `InstanceInterface`) and define their `__i_*` singletons.
3. Branch on `DEVICE_<NAME>` in [src/interface/pdi.h](src/interface/pdi.h) **and** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (the second branch pulls in `<board>_device_config.h`).
4. Add per-board limits in [devices/DeviceConfig.h](devices/DeviceConfig.h).
5. `python3 scripts/DeviceSetup.py -d <board>`.
6. Build [examples/PdiStack](examples/PdiStack/) against the new board.
7. Iterate on optional interfaces (network, storage, WiFi, threading, TLS) one flag at a time.
8. Add the architecture to `library.properties` ([§14.2](#14-build--toolchain)).

Validation checklist: [§2.10](#2-device-layer--porting-guide).

### 16.3 Add a new interface

Full guide: [§3.6](#3-portable-interfaces). The shape:

1. Pick the right group: `drivers/`, `middlewares/`, `modules/`, `threading/`, or top-level.
2. Write [src/interface/pdi/.../i<Name>.h](src/interface/pdi/) with pure virtuals, forward-declared concrete class, `extern Concrete __i_<name>;`.
3. Guard inclusion behind an `ENABLE_*` if your consumers are gated.
4. Add a stub in [devices/mockdevice/](devices/mockdevice/) so off-device builds link.
5. Provide a default implementation under [src/interface/pdi/impl/](src/interface/pdi/impl/) if more than one port would re-implement the same logic.

**The bar:** at least one port should genuinely implement it *differently* from another. A single-port interface is a smell — keep it concrete in the device until the second port shows up.

### 16.4 Add a new service provider

Full guide: [§7.5](#7-service-providers). The shape:

1. New `ENABLE_<NAME>_SERVICE` flag in [devices/DeviceConfig.h](devices/DeviceConfig.h).
2. New `SERVICE_<NAME>` enum value (guarded by the flag) in [src/service_provider/ServiceProvider.h](src/service_provider/ServiceProvider.h).
3. `src/service_provider/<group>/<Name>ServiceProvider.{h,cpp}` deriving from `ServiceProvider`:
   ```cpp
   <Name>ServiceProvider() : ServiceProvider(SERVICE_<NAME>, RODT_ATTR("<Name>")) {}
   bool initService(void* arg) override { /* register tasks, listeners */ return ServiceProvider::initService(arg); }
   ```
4. Conditional include in [src/PdiStack.h](src/PdiStack.h) + `initService(...)` call in [src/PdiStack.cpp](src/PdiStack.cpp), slotted per the **init order rules** ([§7.3](#7-service-providers)).
5. Override `printConfigToTerminal` / `printStatusToTerminal` so the `srvc` CLI sees you.
6. (Optional) Add a CLI command ([§16.8](#168-add-a-new-cli-command)) and/or a web controller ([§16.7](#167-add-a-new-web-page)).
7. (Optional) Persist config via a database table ([§16.6](#166-add-a-new-database-table)).

Anti-patterns: ctor side-effects, direct service-to-service coupling for fan-out (use events), allocating after `setup()`. See [§7.6](#7-service-providers).

### 16.5 Add a new transport

Full guide: [§8.7](#8-transports). The shape:

1. `src/transports/<proto>/<Proto>Client.{h,cpp}` that takes an `iClientInterface*` and exposes `begin / send / receive / end`. No `__database_service`, no `__task_scheduler` — transports parse bytes, period.
2. Build a `<Proto>ServiceProvider` *on top* — the service owns the schedule, config, and event dispatch.
3. One new `ENABLE_*` flag, wired through [PdiStack.h](src/PdiStack.h) / [PdiStack.cpp](src/PdiStack.cpp) the usual way.

The split matters: a transport written this way is reusable from sketches and from other services without dragging the framework in. See HTTP / MQTT / SMTP as references.

### 16.6 Add a new database table

Two paths, depending on whether your table belongs to **the framework** or **your application**.

**Framework table** (must round-trip through every port, surface in `srvc`, be visible to the web portal) — full guide [§6.9](#6-database-layer):

1. Define the struct in a new or existing [src/config/<X>Config.h](src/config/) with the [§4.4](#4-configuration-system) 6-part contract.
2. Pick a free address using the gap calculator.
3. Add a `defItems` entry to each `devices/<board>/config/DBTableSchema.json` that needs it.
4. Regenerate: `python3 scripts/DeviceSetup.py -d <board>`.
5. Add include + global + accessor pair in [DatabaseServiceProvider.{h,cpp}](src/service_provider/database/DatabaseServiceProvider.h) under your `ENABLE_*` flag.
6. Raise `MAX_DB_TABLES` if needed.

**Sketch-local table** (only the application code needs to see it):

- Use the escape hatch from [§15.3](#153-addingdatabasetable--app-side-persistence-without-codegen) — `class MyTable : public DatabaseTable<ADDR, my_struct> {};` directly in the `.ino`. No codegen, no schema edit. The trade-off: not visible to the framework's reflective tools.

### 16.7 Add a new web page

Full guide: [§9.10](#9-web-server). The shape:

1. Add the route constant to [src/webserver/routes/Routes.h](src/webserver/routes/Routes.h).
2. Create `MyPage.h` under [src/webserver/pages/](src/webserver/pages/) holding the body HTML as a `PROG_RODT_ATTR` const string.
3. Create `MyController` under [src/webserver/controllers/](src/webserver/controllers/) deriving from `Controller`, register the route in `boot()` (with `AUTH_MIDDLEWARE` unless deliberately public).
4. Add a value-member of the controller in [src/webserver/WebServer.h](src/webserver/WebServer.h), conditional on the appropriate flag.
5. Add a menu entry on the home page so users can navigate to it.

For sketch-side controllers (no framework edit), see [examples/Dev/AddingController/](examples/Dev/AddingController/AddingController.ino) and [§15.4](#154-addingcontroller--extending-the-web-portal).

Compose the response in three chunked `send(..., send_in_chunks=true)` calls — header / body / footer — to stay under the per-`send` 1.8 KB cap ([§9.7](#9-web-server), [§9.11](#9-web-server)).

### 16.8 Add a new CLI command

Full guide: [§10.11](#10-command-line--terminal). The shape:

1. Add `CMD_NAME_<X>` to [src/service_provider/cmd/commands/CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h).
2. Create [src/service_provider/cmd/commands/<X>Command.h](src/service_provider/cmd/commands/) — a struct derived from `CommandBase` that calls `SetCommand`, `AddOption`s in its ctor and implements `execute(cmd_term_inseq_t)` returning a `cmd_result_t`.
3. Include the header in [CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) and call `CommandBase::RegisterCommand(CMD_NAME_<X>, &<X>Command::Registrar)` from `CommandLineServiceProvider::initService`.

The new command auto-completes via TAB, gets history via ↑/↓ (storage required), and respects CTRL+C / CTRL+Z if you override `executeTermInputAction`.

Constraints to know: command name ≤ 8 chars, ≤ 3 options, option names ≤ 2 chars ([§10.12](#10-command-line--terminal)).

### 16.9 React to an event

Service-to-service coupling for fan-out should go through `__utl_event`, not direct calls:

```cpp
__utl_event.add_event_listener(EVENT_WIFI_CONNECTED, [&](void* arg) {
    // your handler — runs on the inline scheduler lane
});
```

To fire your own:

```cpp
__utl_event.execute_event(EVENT_MY_THING, &payload);   // synchronous
```

Add new event names to [src/config/EventConfig.h](src/config/EventConfig.h). The framework's own events are listed in [§7.4](#7-service-providers).

### 16.10 Call an existing service from a sketch

Every `ENABLE_*`'d service is a global, named per [§1.7](#1-architecture-overview):

```cpp
#include <PdiStack.h>

void setup() {
    PdiStack.initialize();

    // configuration
    wifi_config_table cfg;
    __database_service.get_wifi_config_table(&cfg);
    memcpy(cfg.sta_ssid, "MyNet", 6);
    __database_service.set_wifi_config_table(&cfg);

    // service control
    __wifi_service.scan_aps_and_configure_wifi_station_async(0);

    // GPIO
    __i_dvc_ctrl.gpioMode(DIGITAL_WRITE, 2);
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);

    // scheduling
    __task_scheduler.setInterval([&] {
        __i_dvc_ctrl.getTerminal()->writeln("tick");
    }, 1000, millis());

    // events
    __utl_event.add_event_listener(EVENT_WIFI_CONNECTED, [&](void*) {
        // reactive code
    });
}

void loop() { PdiStack.serve(); }
```

This is the **only** API contract you have to know for application code — every service is reachable through one named global, every singleton is reachable through one `__i_*` symbol, every operation is sync-and-cheap or scheduled-and-deferred.

### 16.11 Where extensions tend to go wrong

A quick scan of the most common ways extensions fail to land cleanly:

- **Constructor-time work.** The single most repeated mistake — covered in [§3.5](#3-portable-interfaces), [§7.6](#7-service-providers), [§13.12](#13-memory--performance-notes). Use `init*` methods, not constructors.
- **Direct cross-service calls instead of events.** If your new service `__x_service` calls into `__y_service` from a callback, you have created a hidden ordering dependency that breaks on init-order changes. Use events for fan-out, direct calls only when the dependency is structural (e.g. `MqttServiceProvider` always needs `__database_service`).
- **Holding pointers into transient buffers.** `CommandBase` option values, web request args, HTTP response bodies are all borrowed pointers ([§10.4](#10-command-line--terminal), [§8.8](#8-transports), [§9.7](#9-web-server)). Copy or call `holdOptionValue` if you need the bytes past the next call.
- **Pulling vendor headers above the device layer.** If your service or interface file `#include <esp_wifi.h>`, the build breaks for every other port. Push the dependency down into `devices/<board>/`.
- **Forgetting the codegen step.** A new framework-level table needs a JSON schema edit *and* the script re-run *and* the `DatabaseServiceProvider.{h,cpp}` edit. Skipping any one of the three leaves an inconsistent state.
- **Ignoring the `ENABLE_*` triple-gate.** A new feature flag must guard (1) the device aggregator include, (2) the orchestrator include, (3) the orchestrator's `initService` call. Miss one and the build either fails to link or carries dead code. See [§4.3](#4-configuration-system).
- **One global per slot.** Two `XServiceProvider` instances overwrite `m_services[SERVICE_X]` silently. Two `<X>Table` globals at the same address are quietly skipped by `Database::register_table`. One per slot, always.

### 16.12 Where this section runs out

What's **not** covered here because it doesn't exist yet (or barely):

- **Per-tag log filtering** — the logger is global-on, per-level only ([§12.8](#12-logger)).
- **Dynamic service registration** — `service_t` is a compile-time enum.
- **Plugin loading** — no runtime modules.
- **Multi-user sessions** — single cookie row, single CLI prompt at a time.
- **Distributed config** — every device owns its own NVM.

These are deliberate trade-offs for the embedded budget, not omissions. If you find yourself wishing for one of them, you may be reaching past the framework's intended sweet spot — at which point the right move is usually a *companion service* (HTTP frontend, MQTT broker, cloud config server) sitting outside the device.

---

## 17. Troubleshooting & FAQ

This section bundles the most common problems contributors and integrators hit, plus answers to questions that recur. Each entry is short by design — the deep explanations live in the section it points to.

### 17.1 Build & flash problems

**`undefined reference to lfs_*` at link time.**
You skipped the LittleFS submodule. From the repo root:
```
git submodule update --init --recursive
```
See [§14.11](#14-build--toolchain).

**`fatal error: DeviceSetup.h: No such file or directory`.**
The setup script hasn't been run. From the [scripts/](scripts/) directory:
```
python3 DeviceSetup.py -d <board>      # esp8266 / esp32 / arduinouno
```
See [§14.3](#14-build--toolchain).

**Build succeeds, but `srvc l` lists no services and the AP never appears.**
`devices/DeviceSetup.h` still points at the wrong device — usually because you switched boards without re-running the setup script. Re-run with the new `-d <board>`. See [§14.11](#14-build--toolchain).

**`multiple definition of __i_<x>` link error.**
Two ports defined the same singleton, or you referenced a `devices/<board>/*.cpp` from somewhere other than its aggregator. Per [§2.4](#2-device-layer--porting-guide), each device translation unit must be reached through exactly one `.cpp`-include chain.

**`fatal error: esp_wifi.h: No such file or directory` building for AVR.**
A vendor-specific header leaked above the device layer. The fix is to push that include down into `devices/<board>/`. See [§16.11](#16-extending-the-framework).

**Library Manager rejects the install or compiles for the wrong arch.**
Library Manager isn't supported today — only manual clone + submodule + script. The three blockers are detailed in [§14.7](#14-build--toolchain).

**Compile error mentioning `pdiutil::function` / `pdiutil::vector` on an obscure target.**
The host compiler is missing GCC's variadic-macro / variadic-template extensions used in [PdiSTL](src/utility/pdistl/). Switch to a GCC-based toolchain. See [§12.9](#12-logger), [§11.11](#11-utility-library).

### 17.2 Boot & runtime problems

**Device boots but `srvc l` shows nothing or factory-resets every 5 s.**
NVM is in an invalid state (corrupt checksum, mismatched schema after a struct edit). With `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` on (the default), the device auto-recovers — wait one reset cycle. If it loops, the table sizes have outgrown the address allocation; see [§6.10](#6-database-layer).

**Boot hangs after `Starting PDIStack !` with no AP.**
WiFi STA connect is timing out. `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` defaults to 1 s ([§4.3.3](#4-configuration-system)) — short. Usually means the configured STA credentials are stale. Hold the flash button 6-7 s to factory-reset (assuming `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET`), reconnect to AP, set fresh credentials.

**`PdiStack.initialize()` runs to completion but `LogI` calls print nothing.**
No `ENABLE_LOG_*` flag is set in [devices/DeviceConfig.h](devices/DeviceConfig.h) — every `Log*` site compiled to nothing. Uncomment at least `ENABLE_LOG_INFO`. See [§12.8](#12-logger).

**Application code runs once and stops.**
Almost always missing `PdiStack.serve()` in `loop()`. Without it the inline scheduler never advances and nothing periodic fires. The minimal sketch is [examples/PdiStack/PdiStack.ino](examples/PdiStack/PdiStack.ino).

**Heap fragmentation after hours/days of uptime.**
Symptoms: `register_task` returning `-1`, `pdiutil::vector` growth failing, transports refusing new connections. You're allocating inside a hot path. The discipline is in [§13.7](#13-memory--performance-notes) — pre-reserve containers, hold-then-free, no `new` after `setup()`.

**`__task_scheduler.register_task` always returns `-1`.**
The slot table (`MAX_SCHEDULABLE_TASKS = 25`) is full. Either you have a legitimate need for more (raise the constant in [src/config/Common.h](src/config/Common.h)) or a service is leaking tasks (`scht` will show the population). See [§5.7](#5-task-scheduler).

**One service starves the others; serve loop feels jittery.**
The scheduler runs **one** inline task per `serve()` iteration ([§5.5](#5-task-scheduler)). If a task does too much work per call, every other task suffers. Split into a state machine ([§5.10](#5-task-scheduler)) or promote to a cooperative lane.

### 17.3 Network / portal problems

**Cannot connect to the `pdiStack` AP.**
Default password is `pdiStack@123` (case-sensitive). Confirm the AP is up via `srvc l` + `srvc s=<id>,q=2` over serial. If still missing, `ENABLE_WIFI_SERVICE` may be off — re-check [devices/DeviceConfig.h](devices/DeviceConfig.h).

**Cannot reach `http://192.168.0.1` from the AP.**
Default subnet is hardcoded in [WifiConfig.h](src/config/WifiConfig.h). If `ENABLE_DYNAMIC_SUBNETTING` is on, the AP may have picked a different subnet; check the IP your phone/laptop got from DHCP and use that subnet's `.1`.

**Login form keeps redirecting to `/login`.**
Cookie is being rejected or session expired (default 300 s). Check the browser actually accepts cookies for the device IP; private-mode windows often don't.

**The page is truncated mid-HTML.**
A `send()` was called without `send_in_chunks=true` and the payload exceeded `PAGE_HTML_MAX_SIZE = 1800`. Compose pages in three chunked sends (header/body/footer) — see [§9.7](#9-web-server) and the stale-comment caveat in [§15.4](#154-addingcontroller--extending-the-web-portal).

**MQTT connects but no callbacks fire.**
For inbound data, confirm `Subscribe` was actually acknowledged by waiting for the `OnSubscribed` callback before publishing on the same topic. `Subscribe` returns success on *enqueue*, not on SUBACK ([§8.8](#8-transports)).

**OTA pings the server but never updates.**
The framework only updates when **the server's `latest` version is strictly newer than the device's `FIRMWARE_VERSION`** ([README §OTA Service](#services), [§7.2.5](#7-service-providers)). Bump `FIRMWARE_VERSION` in [GlobalConfig.h](src/config/GlobalConfig.h) before publishing.

**Email send times out.**
SMTP is plaintext; the framework has **no STARTTLS** ([§8.5](#8-transports)). TLS-only SMTP providers (Gmail SMTP submission, SES) won't work. Use a relay that accepts plain SMTP on the LAN, or a mailtrap-style test sink.

**HTTPS server returns connection-refused / handshake-fail.**
Cert/key not on the device FS, or paths don't match the defaults. Upload via SFTP to `/etc/http/server.crt` and `/etc/http/server.key` (paths overridable via `TLS_DEFAULT_SERVER_*_PATH` in [TlsConfig.h](src/config/TlsConfig.h)). On esp32, the on-device `tls q=1,t=0,l=256,n=<CN>,i=<IPv4>` command can generate them in place; on esp8266, use [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) off-device and upload. Watch for `ENABLE_HTTPS_SERVER_MTLS` builds — they additionally need `/etc/http/client-ca.crt`. See [§7.2.16](#7-service-providers).

**Outbound HTTPS to an unknown CA fails.**
The bundled `m_client` in `PdiStack` defaults to `setVerifyPeer(false)` so first-boot demos work without provisioned trust anchors. For production cross-network paths, edit [src/PdiStack.cpp](src/PdiStack.cpp): drop the `setVerifyPeer(false)` line and uncomment `setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH)`, then upload a CA bundle to `/etc/ssl/ca-bundle.crt`.

**esp8266 TLS handshake stack-overflows / fails on a 16 KB record.**
Two ceilings: (1) BearSSL ECDSA sign needs more than the 4864 B default Arduino cont_t stack — the TLS task runs on a dedicated `TLS_TASK_STACK_SIZE = 6500` B stack; lower at your own risk. (2) `TLS_IBUF_SIZE = 2048` defaults below BearSSL's spec maximum of ~17 KB. Peers that emit 16 KB records will fail the handshake — bump it (and accept the heap cost) only if you can't control the peer.

**HTTPS portal works on esp32 but esp8266 rejects every connection.**
Most likely `ENABLE_NAPT` is also on. The two cannot coexist on esp8266 — both want too much of the same fixed heap. Disable NAPT for HTTPS builds. See [§13.6](#136-the-expensive-features-called-out).

**`EVENT_WIFI_STA_GOT_IP` listener (auto cert-gen) never fires.**
`ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` is gated on both `ENABLE_TLS_CERT_GENERATION` (esp32 only) and a successful STA association. Confirm STA is actually getting an IP via `net ip`; auto-mint won't run on AP-only mode.

### 17.4 CLI / terminal problems

**Telnet / SSH login fails with the right credentials.**
The CLI uses the same login row as the web portal (`login_credential_table`). If you've changed credentials via the web, telnet/SSH inherits them. If history `srvc s=<auth>,q=2` shows defaults, factory reset.

**SSH won't accept any client.**
You probably never generated a key pair. From the CLI:
```
ssh q=1,t=1
```
(`q=1` is `SSH_COMMAND_QUERY_KEYGEN`, `t=1` selects the algorithm — see [SshConfig.h](src/config/SshConfig.h)).

**`scp -s …` is slow.**
By design — the framework streams SFTP in small bolus chunks (≤256 B) and has overwrite overhead on flash filesystems. Expected throughput 0.2-1 KB/s ([§7.2.14](#7-service-providers), [§10.9](#10-command-line--terminal)). Use OTA, not SFTP, for firmware.

**Two telnet clients can't connect simultaneously.**
Only one CLI session is supported at a time across all transports — telnet, SSH, and serial share the bound terminal ([§10.8](#10-command-line--terminal)).

**`fwrite` won't exit.**
Press **ESC** to finalize (commit + exit). Per [README §Terminal](#terminal) and [§10.7](#10-command-line--terminal).

**TAB autocomplete works but ↑/↓ history doesn't.**
History is persisted to `/.term_history` — only available when `ENABLE_STORAGE_SERVICE` is on. Autocomplete uses the in-RAM command registry and works regardless. See [§10.6](#10-command-line--terminal).

**Command name longer than 8 chars is silently truncated.**
`CMD_SIZE_MAX = 8`. Shorten the name. See [§10.12](#10-command-line--terminal).

### 17.5 Frequently asked questions

**Q. Can I run multiple `PDIStack` instances?**
No. The framework relies on a single global `PdiStack` plus one singleton per interface and per service. There's no path to multi-instance — and on the target memory budgets, no point.

**Q. Can I use `String` (Arduino) instead of `pdiutil::string`?**
In sketch code, yes. In framework code, no — `String`'s allocator differs by core and breaks the off-AVR portability the framework relies on. See [§11.9](#11-utility-library).

**Q. Why are command names so short?**
`CMD_SIZE_MAX = 8` and `CMD_OPTION_SIZE_MAX = 3` are sized for AVR-class RAM. Loosening them costs RAM proportional to `MAX_SCHEDULABLE_TASKS × CMD_OPTION_MAX × CMD_OPTION_SIZE_MAX`. The trade-off was made for UNO.

**Q. Why is SSH so heavy?**
Ed25519 keypair + Curve25519 ECDH + AES-CTR streaming + per-session protocol state. Roughly +150 KB flash and +8-16 KB heap per session ([§13.5](#13-memory--performance-notes)). It's why ESP32 is the recommended target for SSH builds.

**Q. Can the device be configured without flashing — e.g. from a USB serial bootstrap?**
Yes. After the first boot, every persisted config in NVM can be changed via the web portal, the CLI, or another sketch that calls `__database_service.set_*_table`. Flash only changes *defaults* ([§4.8](#4-configuration-system)).

**Q. Where do logs go?**
Serial at 115200 only, by default. The framework doesn't ship file/telnet/SSH/syslog/HTTP routing for logs — implement a custom `LoggerInterface` if you need fan-out ([§12.6](#12-logger)).

**Q. How do I enable TLS?**
TLS ships in-tree as of the last release. Set `ENABLE_TLS_SERVICE` in [devices/DeviceConfig.h](devices/DeviceConfig.h) — esp8266 uses BearSSL, esp32 uses mbedTLS. Add `ENABLE_HTTPS_SERVER` for the inbound web portal (cert/key at `/etc/http/server.{crt,key}`), `ENABLE_HTTPS_SERVER_MTLS` for client-cert auth (`/etc/http/client-ca.crt`), `ENABLE_TLS_CERT_GENERATION` + `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` (esp32 only) for on-boot auto-mint. Cost is significant — see [§13.5](#135-what-each-feature-actually-costs). On esp8266 `ENABLE_TLS_SERVICE` cannot coexist with `ENABLE_NAPT`. Full reference: [§7.2.16](#7-service-providers).

**Q. How do I provision certs?**
Two paths. **On-device (esp32 only):** `tls q=1,t=0,l=256,n=device.local,i=192.168.1.50` from the CLI generates and stores a self-signed EC P-256 cert + key at the default `TlsConfig.h` paths. **Off-device (any board):** `python3 scripts/GenTlsCerts.py --dns device.local --ip 192.168.1.50` produces files under `certs/`, then upload via SFTP. For a stable dev CA, run with `--gen-ca` once and use `--ca-cert`/`--ca-key` for each device cert thereafter — every device that trusts that CA bundle then trusts every device cert it signs.

**Q. Is there a Wokwi / on-host simulator?**
Not bundled. `devices/mockdevice/` is a header-only stub that lets the framework *compile* off-device for static analysis, but it doesn't simulate behaviour. For interactive testing, flash to real hardware.

**Q. How do I unit-test framework code?**
The interface-based design supports it: mock the `i*Interface` your unit depends on and link against the unit's `.cpp`. The framework doesn't ship a test harness — bring your own (CppUTest, Catch2, …). PdiSTL works on host x86 with GCC.

**Q. What's the upgrade path from old `esp8266-framework` (Suraj151's prior repo)?**
The README references the older repo's screenshots. Migration is mostly mechanical — rename `__i_dvc_ctrl` calls, replace `String` with `pdiutil::string`, follow the [§2.9 device port guide](#2-device-layer--porting-guide) if you had a custom board.

**Q. Why does the project use `dot_a_linkage=true`?**
So unreferenced services are dropped by the linker. Without it, every `__<x>_service` instance would land in the final binary regardless of `ENABLE_*` flags ([§14.2](#14-build--toolchain)).

**Q. Does the framework receive security updates?**
Crypto primitives are upstream (`ed25519`, `curve25519`, `aes`, `sha*`) — vendored copies. There's no automated CVE tracking. If you ship a product on it, mirror the upstream sources and watch their release feeds yourself.

**Q. Can I disable the auto-factory-reset behaviour?**
Yes — comment out `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` in [devices/DeviceConfig.h](devices/DeviceConfig.h). Then invalid NVM halts further service initialization until manually cleared, which is sometimes what you want during development.

**Q. Where do I report bugs / file issues?**
GitHub: <https://github.com/Suraj151/pdi-framework>.

### 17.6 When you're really stuck

The best signal-to-noise loop on an unknown problem:

1. **Enable logs.** Uncomment `ENABLE_LOG_ALL` in [devices/DeviceConfig.h](devices/DeviceConfig.h), flash, watch serial @ 115200.
2. **List services.** `srvc l` over the terminal — confirms what actually booted.
3. **Print a service's config / status.** `srvc s=<id>,q=1` / `srvc s=<id>,q=2`.
4. **List active tasks.** `scht` — column `exc_ms` is your CPU-hog detector.
5. **Watch a task over time.** `watch c=scht; i=2000; n=10`.
6. **Check NVM integrity.** A `srvc s=<db>,q=2` reports DB validity; outside that, an `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` build will reset every 5 s if NVM is bad.
7. **Reboot.** `reboot` — the explicit version, since pulling power loses serial output.

If none of those localise the issue, open a GitHub issue with: device + board-package version, the relevant `ENABLE_*` flags, the serial log up to and including the failure, and the exact command sequence that reproduces.

