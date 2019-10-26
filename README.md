# The `mlan/asterisk` repository

![travis-ci test](https://img.shields.io/travis/mlan/docker-asterisk.svg?label=build&style=popout-square&logo=travis)
![image size](https://img.shields.io/microbadger/image-size/mlan/asterisk.svg?label=size&style=popout-square&logo=docker)
![docker stars](https://img.shields.io/docker/stars/mlan/asterisk.svg?label=stars&style=popout-square&logo=docker)
![docker pulls](https://img.shields.io/docker/pulls/mlan/asterisk.svg?label=pulls&style=popout-square&logo=docker)

THIS DOCUMENT IS UNDER DEVELOPMENT AND CONTAIN ERRORS

This (non official) repository provides dockerized PBX.

## Features

Feature list follows below

- [websms](srs/websms/README.md) service sends and receives HTTP SMS
- Asterisk PBX
- php webhook for (incoming) SMS http ISTP origination
- dialplan php (outgoing) SMS http ISTP termination
- dialplan ISTP originating (incoming) SIP voice call
- dialplan ISTP termination (outgoing) SIP voice call
- Autoban, an automatic firewall
- Alpine Linux

## Tags

The breaking.feature.fix [semantic versioning](https://semver.org/)
used. In addition to the three number version number you can use two or
one number versions numbers, which refers to the latest version of the 
sub series. The tag `latest` references the build based on the latest commit to the repository.

The `mlan/asterik` repository contains a multi staged built. You select which build using the appropriate tag from `mini`, `base`, `full` and `xtra`. The image `mini` only contain Asterisk.
To exemplify the usage of the tags, lets assume that the latest version is `1.0.0`. In this case `latest`, `1.0.0`, `1.0`, `1`, `full`, `full-1.0.0`, `full-1.0` and `full-1` all identify the same image.

# Usage

Often you want to configure Asterisk and its components. There are different methods available to achieve this. Moreover docker volumes or host directories with desired configuration files can be mounted in the container. And finally you can `docker exec` into a running container and modify configuration files directly.

If you want to test the image right away, probably the best way is to use the `Makefile` that comes with this repository.

To build, and then start a test container you simply have to `cd` into the repository directory and type

```bash
make build test-up
```

The you can connect to the asterisk command line interface (CLI) running inside the container by typing

```bash
make test-cli
```

From the Asterisk CLI you can type

```bash
pjsip show endpoints
```

to see the endpoints (soft phones) that are configured in the `/etc/asterisk/pjsip_wizard.conf` configuration file that comes with the image by default.

When you are done testing you can destroy the test container by typing

```bash
make test-down
```

## Docker compose example

An example of how to configure an VoIP SIP server using docker compose is given below.

```yaml
version: '3.7'

services:
  tele:
    image: mlan/asterisk
    restart: unless-stopped
    cap_add:
      - net_admin
      - net_raw
    networks:
      - proxy
    ports:
      - "80:80"
      - "5060:5060/udp"
      - "10000-10099:10000-10099/udp"
    environment:
      - SYSLOG_LEVEL=4
    volumes:
      - tele-conf:/srv
volumes:
  tele-conf:
```

This repository WILL contain a `demo` directory which hold the `docker-compose.yml` file as well as a `Makefile` which might come handy. From within the `demo` directory you can start the container simply by typing:

# WebSMS

The [websms](src/websms/doc/websms.md) service is described [here](src/websms/doc/websms.md).

## Autoban, automatic firewall

The Autoban service listens to Asterisk security events on the AMI interface. Autoban is activated if there is an `autoban.conf` file and that the parameter `enabled` within is not set to `no`. When one of the `InvalidAccountID`, `InvalidPassword`, `ChallengeResponseFailed`, or `FailedACL` events occur Autoban start to watch the source IP address for `watchtime` seconds. If more than `maxcount` security events occurs within this time, all packages from the source IP address is dropped for `jailtime` seconds. When the `jailtime` expires packages are gain accepted from the source IP address, but for additional `watchtime` seconds this address is on "parole". Is a security event be detected from this address during the "parole" period it is immediately blocked again, for a progressively longer time. This progression is configured by `repeatmult`, which determines how many times longer the IP is blocked. To illustrate, first assume `jailtime=20m` and `repeatmult=6`, then the IP is blocked 20min the first time, 2h (120min) the second, 12h (720min) the third, 3days (4320min) the forth and so on. If no security event is detected during the "parole" the IP is no longer being watched.

#### `autoban.conf`

```ini
[asmanager]
server     = 127.0.0.1
port       = 5038
username   = autoban
secret     = 6003.438

[autoban]
enabled    = yes
maxcount   = 10
watchtime  = 20m
jailtime   = 20m
repeatmult = 6
```

The AMI interface is configured in  `autoban.conf` and `manager.conf`. For security reasons use `bindaddr=127.0.0.1`  and change the `secret` (in both files).

#### `manager.conf`

```ini
[general]
enabled  = yes
bindaddr = 127.0.0.1
port     = 5038

[autoban]
secret   = 6003.438
read     = security
write    =
```

Autoban uses nftables which does the actual package filtering. nftables needs the `NET_ADMIN` and `NET_RAW` capabilities to function, which you provide by issuing  `--cap-add=NET_ADMIN --cap-add=NET_RAW`.

You can watch the status of the nftable firewall by, from within the container, typing

```bash
nft list ruleset
```



## Environment variables

When you create the `mlan/asterisk` container, you can configure the services by passing one or more environment variables or arguments on the docker run command line. Once the services has been configured a lock file is created, to avoid repeating the configuration procedure when the container is restated. In the rare event that want to modify the configuration of an existing container you can override the default behavior by setting `FORCE_CONFIG` to a no-empty string.

## Configuration files

Asterisk and its modules are configured using several configuration files which are typically found in `/etc/asterisk`. The  `/mlan/astisk` image provides a collection of configuration files which can serve as starting point for your system. We will outline how we intend the default configuration files are structured.

### Functional

Some of the collection of configuration files provided does not contain any user specific data and might initially be left unmodified. These files are:

| File name        | Description                                                  |
| ---------------- | ------------------------------------------------------------ |
| acl.conf         |                                                              |
| asterisk.conf    | asterisk logging, directory structure                        |
| ccss.conf        |                                                              |
| cli_aliases.conf | command line interface aliases convenience                   |
| extensions.conf  | dialplan how incoming and outgoing calls and messages are handled |
| features.conf    | activation of special features                               |
| indications.conf | dial tone local                                              |
| logger.conf      | logfiles                                                     |
| modules.conf     | activation of modules                                        |
| musiconhold.conf | music on hold directory                                      |
| pjproject.conf   | pjsip installation version                                   |

### Personal

| File name               | Description                                                  |
| ----------------------- | ------------------------------------------------------------ |
| extensions-globals.conf | Defines SIP trunk endpoint                                   |
| minivm.conf             | Define mail sever URL and authentication credentials which voice mail email notifications will be sent |
| pjsip.conf              | Defines SIP transport, protocol, port, host URL              |
| pjsip_wizard.conf       | Defines endpoints, soft-phones, users, sip trunk             |
| rtp.conf                | Define RTP port range                                        |
| sms.conf                | Define HTTP SMS, incoming and outgoing                       |

### `pjsip_wizard.conf` soft-phones and trunks

### `extensions-local.conf ` sms termination

### `pjsip.conf`,  `rtp.conf` network

### `minivm.conf` voice-mail

#### `pjsip-local.conf`

```ini
;================================ GLOBAL ==
[global]
type = global
user_agent = Platform PBX

;================================ TRANSPORTS ==
;
[transport]
type = transport
protocol = udp
bind = 0.0.0.0:5060
domain = example.com
external_media_address = example.com
external_signaling_address = example.com
tos = cs3
cos = 3
```

#### `extensions-local.conf`

```ini
;================================ globals =====================================
; include file providing dialing texting options used in context globals
;
;================================ dialing
[globals]
DIAL_TIMEOUT =,30
TRUNK_ENDPOINT = trunk_example
;================================ voice mail
VOICEMAIL_TEMPLATE =,en_US_email
VOICEMAIL_RECGAINDB =,g(12)
;================================ sms
; Full path to SMS app
APP_SMS = /usr/share/php7/sms.php

;================================ entries =====================================
; Calls enter the dialplan in one of these entries
;
[dp_entry_user_calling]

[dp_entry_trunk_calling]

[dp_entry_user_texting]

[dp_entry_trunk_texting]

[dp_entry_channel_open]
```

#### `pjsip_wizard.conf`

```ini
;================================ TEMPLATES ==

[trunk_defaults](!)
type = wizard
transport = transport
endpoint/context = dp_entry_trunk_calling
endpoint/allow = !all,ulaw
endpoint/direct_media=no
endpoint/rewrite_contact=yes
endpoint/rtp_symmetric=yes
endpoint/allow_subscribe = no
endpoint/send_rpid = yes
endpoint/send_pai = yes
aor/qualify_frequency = 60


[outbound_defaults](!,trunk_defaults)
type = wizard
sends_auth = yes

[inbound_defaults](!,trunk_defaults)
type = wizard
accepts_auth = yes


[user_defaults](!)
type = wizard
transport = transport
accepts_registrations = yes
accepts_auth = yes
has_hint = yes
hint_context = dp_lookup_user
endpoint/context = dp_entry_user_calling
endpoint/message_context = dp_entry_user_texting
endpoint/from_domain = example.com
endpoint/allow_subscribe = yes
endpoint/tos_audio=ef
endpoint/tos_video=af41
endpoint/cos_audio=5
endpoint/cos_video=4
endpoint/send_pai = yes
endpoint/allow = !all,ulaw
endpoint/rtp_symmetric = yes
endpoint/trust_id_inbound = yes
endpoint/language = en
aor/max_contacts = 5
aor/remove_existing = yes

;================================ SIP ITSP ==

[trunk_example](outbound_defaults)
remote_hosts = host.example.com
outbound_auth/username = user
outbound_auth/password = password

;================================ SIP USERS ==

[john.doe](user_defaults)
hint_exten = +12025550160
endpoint/callerid = John Doe <+12025550160>
endpoint/mailboxes = john.doe@example.com
endpoint/from_user = +12025550160
inbound_auth/username = john.doe
inbound_auth/password = password

[jane.doe](user_defaults)
hint_exten = +12025550183
endpoint/callerid = Jane Doe <+12025550183>
endpoint/mailboxes = jane.doe@example.com
endpoint/from_user = +12025550183
inbound_auth/username = jane.doe
inbound_auth/password = password
```

#### `rtp.conf`

```ini
[general]
; RTP start and RTP end configure start and end addresses
; docker will stall if we open a large range of ports, since it runs a
; a proxy process of each exposed port
rtpstart = 10000
rtpend   = 10099
;
; Strict RTP protection will drop packets that have passed NAT. Disable to allow
; remote endpoints connected to LANs.
;
strictrtp = no
```

####`sms.conf`

This file hold user customization of http sms
originating and termination service.

```ini
[sms]
sms_host             = api.example.com
sms_path             = /sms/send/
sms_auth_user        = user
sms_auth_passwd      = passwd

[smsd]
smsd_exten_context   = dp_entry_channel_open
smsd_message_context = dp_entry_trunk_texting
```

#### `minivm.conf`

```ini
[general]
;  DESCRIPTION
;    This script simplifies smtps connections for sendmail
;  USAGE
;    minivm-send -H <host:port> [OPTIONS] < message
;  OPTIONS
;    -e                 Also send log messages to stdout
;    -f <from addr>     For use in MAIL FROM
;    -H <host:port>     Mail host/ip and port
;    -h                 Print this text
;    -P <protocol>      Choose from: smtp, smtps, tls, starttls
;    -p <password>      Mail host autentication clear text password
;    -u <username>      Mail host autentication username
;
mailcmd = minivm-send -H mx.example.com:587 -P starttls -u username -p password -f voicemail-noreply@example.com

...
```



## Persistent storage

By default, docker will store the configuration and run data within the container. This has the drawback that the configurations and queued and quarantined mail are lost together with the container should it be deleted. It can therefore be a good idea to use docker volumes and mount the run directories and/or the configuration directories there so that the data will survive a container deletion.

To facilitate such approach, to achieve persistent storage, the configuration and run directories of the services has been consolidated to `/srv/etc` and `/srv/var` respectively. So if you to have chosen to use both persistent configuration and run data you can run the container like this:

```
docker run -d --name pbx-mta -v pbx-mta:/srv -p 127.0.0.1:25:25 mlan/asterisk
```

## Initialization procedure

The `mlan/asterisk` image is compiled without any configuration files. When a container is created using the `mlan/asterisk` image default configuration files are copied to the configuration directory `etc/asteroisk` if it is found to be empty. This behavior is intended to support the following initialization procedures.

In scenarios where you already have a collection of configuration files on a docker volume, start/create a `mlan/asterisk` container with this volume mounted. At startup these configuration files are recognized and left untouched and asterisk is stated. The same will happen when the container is restarted. 

In a scenario where we don't have any configuration files yet we start/create a want to start `mlan/asterisk` container with an empty target volume. At startup the default configuration files will be copied to the mounted volume. Now you can edit these configuration files to your liking either from within the container or directly from the volume mounting point on the  docker host. At consecutive startup these configuration files are recognized and left untouched and asterisk is stated.

## Logging `SYSLOG_LEVEL`

The level of output for logging is in the range from 0 to 8. 1 means emergency logging only, 2 for alert messages, 3 for critical messages only, 4 for error or worse, 5 for warning or worse, 6 for notice or worse, 7 for info or worse, 8 debug. Default: `SYSLOG_LEVEL=4`

