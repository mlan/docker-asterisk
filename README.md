# The `mlan/asterisk` repository

![travis-ci test](https://img.shields.io/travis/mlan/docker-asterisk.svg?label=build&style=flat-square&logo=travis)
![docker build](https://img.shields.io/docker/cloud/build/mlan/asterisk.svg?label=build&style=flat-square&logo=docker)
![image size](https://img.shields.io/docker/image-size/mlan/asterisk.svg?label=size&style=flat-square&logo=docker)
![docker stars](https://img.shields.io/docker/stars/mlan/asterisk.svg?label=stars&style=flat-square&logo=docker)
![docker pulls](https://img.shields.io/docker/pulls/mlan/asterisk.svg?label=pulls&style=flat-square&logo=docker)

This (non official) repository provides dockerized asterisk PBX.

## Features

Feature list follows below

- [Asterisk](http://www.asterisk.org/) powering IP PBX systems and VoIP gateways.
- [PrivateDial](src/privatedial/README.md), an easily customized asterisk configuration
- [WebSMS](srs/websms/README.md), send and receive Instant Messages, SMS over HTTP
- [AutoBan](src/autoban/README.md), a built in intrusion detection and prevention system
- Small image size based on [Alpine Linux](https://alpinelinux.org/)
- [Demo](#docker-compose-example) based on `docker-compose.yml` and `Makefile` files
- Automatic integration of [Let’s Encrypt](https://letsencrypt.org/) LTS certificates using the reverse proxy [Traefik](https://docs.traefik.io/)
- Consolidated configuration and run data under `/srv` to facilitate persistent storage
- [Container audio](#container-audio) using the PulseAudio unix socket of the host.
- Health check
- Log directed to docker daemon with configurable level
- Multi-staged build providing the images `mini`, `base`, `full` and `xtra`

## Tags

The breaking.feature.fix [semantic versioning](https://semver.org/)
used. In addition to the three number version number you can use two or
one number versions numbers, which refers to the latest version of the 
sub series. The tag `latest` references the build based on the latest commit to the repository.

The `mlan/asterisk` repository contains a multi staged built. You select which build using the appropriate tag from `mini`, `base`, `full` and `xtra`. The image with the tag `mini` only contains Asterisk it self.
The `base` tag also include support for TLS, logging, WebSMS and AutoBan. `full` adds support for console audio. The `xtra` tag includes all Asterisk packages.

 To exemplify the usage of the tags, lets assume that the latest version is `1.0.0`. In this case `latest`, `1.0.0`, `1.0`, `1`, `full`, `full-1.0.0`, `full-1.0` and `full-1` all identify the same image.

# Usage

Often you want to configure Asterisk and its components. There are different methods available to achieve this. Moreover docker volumes or host directories with desired configuration files can be mounted in the container. And finally you can `docker exec` into a running container and modify configuration files directly.

If you want to test the image right away, probably the best way is to clone the [github](https://https://github.com/mlan/docker-asterisk) repository and run the demo therein.

```bash
git clone https://github.com/mlan/docker-asterisk.git
```

## Docker compose example

An example of how to configure an VoIP SIP server using docker compose is given below.

```yaml
version: '3'

services:
  tele:
    image: mlan/asterisk
    network_mode: bridge                    # Only here to help testing
    cap_add:
      - sys_ptrace                          # Only here to help testing
      - net_admin                           # Allow NFT, used by AutoBan
      - net_raw                             # Allow NFT, used by AutoBan
    ports:
      - "${SMS_PORT-8080}:${WEBSMSD_PORT:-80}" # WEBSMSD port mapping
      - "5060:5060/udp"                     # SIP UDP port
      - "5060:5060"                         # SIP TCP port
      - "5061:5061"                         # SIP TLS port
      - "10000-10099:10000-10099/udp"       # RTP ports
    environment:
      - SYSLOG_LEVEL=${SYSLOG_LEVEL-4}      # Logging
      - HOSTNAME=${TELE_SRV-tele}.${DOMAIN-docker.localhost}
      - PULSE_SERVER=unix:/run/pulse/socket # Use host audio
      - PULSE_COOKIE=/run/pulse/cookie      # Use host audio
      - WEBSMSD_PORT=${WEBSMSD_PORT-80}     # WEBSMSD internal port
    volumes:
      - tele-conf:/srv                      # Persistent storage
      - ./pulse:/run/pulse:rshared          # Use host audio
      - /etc/localtime:/etc/localtime:ro    # Use host timezone

volumes:
  tele-conf:                                # Persistent storage
```

This repository contains a `demo` directory which hold the `docker-compose.yml` file as well as a `Makefile` which might come handy. From within the `demo` directory you can start the container simply by typing:

```bash
make up
```

The you can connect to the asterisk command line interface (CLI) running inside the container by typing

```bash
make cmd
```

From the Asterisk CLI you can type

```bash
pjsip show endpoints
```

to see the endpoints (soft phones) that are configured in the `/etc/asterisk/pjsip_wizard.conf` configuration file that comes with the image by default.

When you are done testing you can destroy the test container by typing

```bash
make destroy
```

## Environment variables

Despite the fact that Asterisk is configured using configuration files, there is a handful for environmental variables that controls the behavior of other functions within the `mlan/asterisk` container. These functions are logging and the management of TLS certificates.

| Variable                                  | Default         | Description                                                  |
| ----------------------------------------- | --------------- | ------------------------------------------------------------ |
| [SYSLOG_LEVEL](#logging-syslog_level)     | 4               | Logging level, from 0 to 8. 0 is minimal, where as, 8 is maximal log outputs. |
| SYSLOG_OPTIONS                            | -SDt            | S: smaller output, D: drop duplicates, t: Strip client-generated timestamps. |
| [ACME_FILE](#acme_file)                   | /acme/acme.json | File that contains TLS certificates, provided by [Let's encrypt](https://letsencrypt.org/) using [Traefik](https://docs.traefik.io/). |
| HOSTNAME                                  | $(hostname)     | Used to identify the relevant TLS certificates in ACME_FILE  |
| [TLS_CERTDAYS](#tls_keybits-tls_certdays) | 30              | Self-signed TLS certificate validity duration in days.       |
| [TLS_KEYBITS](#tls_keybits-tls_certdays)  | 2048            | Self-signed TLS key length in bits.                          |
| [WEBSMSD_PORT](#websmsd_port)             | 80              | PHP web server port, used by WebSMS. Undefined or non-numeric, will disable the PHP web server. |

## Configuration files

Asterisk and its modules are configured using several configuration files which are typically found in `/etc/asterisk`. The `/mlan/asterisk` image include collection of sample configuration files which can serve as starting point for your system.

Some of the collection of configuration files provided does not contain any user specific data and might initially be left unmodified. These files are:

| File name        | Description                                                  |
| ---------------- | ------------------------------------------------------------ |
| alsa.conf        | Open Sound System (ALSA) console driver configuration        |
| asterisk.conf    | Asterisk global configuration including; debug, run-as-user and directory structure |
| ccss.conf        | Call Completion Supplementary Services configuration         |
| cli_aliases.conf | Asterisk Command Line Interface aliases                      |
| features.conf    | Call Features (transfer, monitor/mixmonitor, etc) configuration |
| indications.conf | Location specific tone indications                           |
| logger.conf      | Logging configuration                                        |
| modules.conf     | Module Loader configuration                                  |
| musiconhold.conf | Music on Hold configuration                                  |
| pjproject.conf   | Common pjproject options                                     |
| rtp.conf         | RTP configuration including port range                       |

The configuration files mentioned above are perhaps not the ones that require the most attention. The configuration files defining key aspects of the Asterisk server like; the call flow and SIP trunk and phone details is the concern of the add-on [PrivateDial](#privatedial). Please refer to its separate [documentation](src/privatedial/README.md) for details.

## Persistent storage

By default, docker will store the configuration and run data within the container. This has the drawback that the configuration and state of the applications are lost together with the container should it be deleted. It can therefore be a good idea to use docker volumes and mount the configuration and spool directories directories there so that the data will survive a container deletion.

To facilitate such approach, to achieve persistent storage, the configuration and spool directories of the services has been consolidated under `/srv`. The applications running inside the container still finds files in their usual locations since symbolic links are there pointing back to `/srv`. With this approach simply mounting a docker volume at `/srv` let you keep application configuration and state persistent.

The volume `tele-conf` in the [demo](#docker-compose-example) described above achieves this. An other way would be to run the container like this:

```
docker run -d -v tele-conf:/srv ... mlan/asterisk
```

## Initialization procedure

The `mlan/asterisk` image is built with sample configuration files placed in a seeding directory. The directories where the applications look for configuration files are empty. When the container is started the configuration directory, `etc/asterisk` , is scanned. If it is found to be empty, sample configuration files from the seeding directory are copied to the configuration directory.

The initialization procedure will leave any existing configuration untouched. If configuration files are found, nothing is copied or modified during start up. Only when `etc/asterisk` is found to be empty, will seeding files be copied. This behavior should keep your conflagration safe also when upgrading to a new version of the `mlan/asterisk` image. Should a new version of the `mlan/asterisk` image come with interesting update to any sample configuration file, it need to manually be copied or merged with the present configuration files.

## Logging `SYSLOG_LEVEL`

The level of output for logging is in the range from 0 to 8. 1 means emergency logging only, 2 for alert messages, 3 for critical messages only, 4 for error or worse, 5 for warning or worse, 6 for notice or worse, 7 for info or worse, 8 debug. Default: `SYSLOG_LEVEL=4`

# Networking
SIP networking is quite complex and there is many things that can go wrong. We try to offer some guidance by discussing some fundamentals here.

## SIP protocol

The [Session Initiation Protocol (SIP)](https://en.wikipedia.org/wiki/Session_Initiation_Protocol) is a [signaling protocol](https://en.wikipedia.org/wiki/Signaling_protocol) used for initiating, maintaining, and terminating real-time sessions that include voice, video and messaging applications.

### Transports, UDP, TCP and TLS

SIP is designed to be independent of the underlying [transport layer](https://en.wikipedia.org/wiki/Transport_layer) protocol, and can be used with the [User Datagram Protocol](https://en.wikipedia.org/wiki/User_Datagram_Protocol) (UDP), the [Transmission Control Protocol](https://en.wikipedia.org/wiki/Transmission_Control_Protocol) (TCP), and the [Stream Control Transmission Protocol](https://en.wikipedia.org/wiki/Stream_Control_Transmission_Protocol) (SCTP). For secure transmissions of SIP messages over insecure network links, the protocol may be encrypted with [Transport Layer Security](https://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS). For the transmission of media streams (voice, video) the [Session Description Protocol](https://en.wikipedia.org/wiki/Session_Description_Protocol) (SDP) payload carried in SIP messages typically employs the [Real-time Transport Protocol](https://en.wikipedia.org/wiki/Real-time_Transport_Protocol) (RTP) or the [Secure Real-time Transport Protocol](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol) (SRTP).

When sending an audio stream it is far better to lose a packet than to have a packet retransmitted, causing excessive jitter in the packet timing. Audio is real-time and requires a protocol like UDP to work correctly. Packet loss does not break audio, it only reduces the quality. Therefore it is no surprise that RTP is built on top of UDP; an connection-less protocol.

TCP is a connection-oriented protocol as it establishes an end to end connection between computers before transferring the data. TCP is therefore, by contrast, ideal for SIP signaling. The somewhat unexpected fact that most SIP communication uses UDP should not discourage you from shooing TCP when possible. TCP often provide more stable contacts to endpoints/phones than UDP.

TLS, operating as an application protocol layered directly over TCP, protects against attackers who try to listen on the signaling link but it does not provide end-to-end security to prevent espionage and law enforcement interception, as the encryption is only hop-by-hop and every single intermediate proxy has to be trusted.

The media streams which are separate connections from the signaling stream, may be encrypted with the [Secure Real-time Transport Protocol](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol) (SRTP). The key exchange for SRTP is performed with [SDES](https://en.wikipedia.org/wiki/SDES), or with [ZRTP](https://en.wikipedia.org/wiki/ZRTP).

### Ports, 5060, 5061 and 10000-20000

SIP clients typically use TCP or UDP on [port numbers](https://en.wikipedia.org/wiki/Port_number) 5060 or 5061 for SIP traffic to servers and other endpoints. Port 5060 is commonly used for non-encrypted signaling traffic whereas port 5061 is typically used for traffic encrypted with [Transport Layer Security](https://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS).

RTP uses a dynamic port range (and is only ever UDP), generally between 10000-20000. This range can usually be customized on the client to suit differing firewall configurations or other concerns.

## Docker, iptables and docker-proxy

When publishing ports on a docker container, using default `bridge` networking, two things happen; routing rules are updated in the Linux kernel firewall and a proxy processes are stated. The current networking implementation in docker (19.03.5), built on the aged [iptables](https://en.wikipedia.org/wiki/Iptables) and docker-proxy, cannot publish port ranges, but instead publish them one by one. This is problematic since normally RTP uses the port range 10000-20000 and updating the firewall 10001 times and staring 10001 proxy processes can take unacceptable long time (minutes) and often prevents the container to be stated all together. Long term this limitation will be addressed since it is inevitable that docker networking will have to be redesigned. But right now we have to work around this imitation and we describe two ways to address this here.

First, you can use the `host` network mode (`docker run --network host ...`). The host network mode does not use the docker-proxy and does not need to set up routing rules in the firewall. The downside is that a container stated in this way cannot communicate on any of the docker networks.

Second, you can stay with the default or user-defined `bridge` mode and instead limit the RTP port range to something manageable say 10000-10099 up to 10000-10999. This actually seems to work in practice, at least with some trunk providers (ITSP). The RTP port range is configured in `rtp.conf` `rtpstart = 10000, rtpend = 10099`.

## Network address translation (NAT)

[Network address translation (NAT)](https://en.wikipedia.org/wiki/Network_address_translation) is a method of remapping one IP [address space](https://en.wikipedia.org/wiki/Address_space) into another by modifying [network address](https://en.wikipedia.org/wiki/Network_address) information in the [IP header](https://en.wikipedia.org/wiki/IP_header) of packets while they are in transit across a traffic [routing device](https://en.wikipedia.org/wiki/Router_(computing)). Here two network environments often results in NAT being used. First, the SIP server we deploy using `mlan/asterisk` runs inside a docker container and depending on what type of network we choose docker will start a proxy process.

### Sip host and domain name

The host name need to be set in three files:

- docker/docker-compose, e.g., `docker run -e HOSTNAME=sip.example.com ...`
- `pjsip-local.conf` `domain = sip.example.com`, `external_media_address = sip.example.com`, and `external_signaling_address = sip.example.com`
- `pjsip_wizard.conf` `endpoint/from_domain = sip.example.com`

With these settings you should not need ICE, STUN or TURN.

## Security - Privacy and integrity

[Transport Layer Security](http://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS) provides encryption for call signaling. A excellent guide for setting up TLS between Asterisk and a SIP client, involving creating key files, modifying Asterisk's SIP configuration to enable TLS, creating a SIP endpoint/user that's capable of TLS, and modifying the SIP client to connect to Asterisk over TLS, can be found here [Secure Calling Tutorial](https://wiki.asterisk.org/wiki/display/AST/Secure+Calling+Tutorial). 

The PrivateDial configuration is already set up to provide both UDP and TCP. TLS and SDES SRTP are also prepared, but a [TLS/SSL server certificate](https://en.wikipedia.org/wiki/Public_key_certificate) and key are needed for their activation. If the certificate and key do not exist when the container starts a [self-signed certificate](https://en.wikipedia.org/wiki/Self-signed_certificate) and private key will automatically be generated.

#### `TLS_KEYBITS`, `TLS_CERTDAYS`

TODO!

 `TLS_KEYBITS=2048`, `TLS_CERTDAYS=30`.

There is also a mechanism to use ACME lets encrypt certificates.

### Let’s Encrypt LTS certificates using Traefik

Let’s Encrypt provide free, automated, authorized certificates when you can demonstrate control over your domain. Automatic Certificate Management Environment (ACME) is the protocol used for such demonstration. There are many agents and applications that supports ACME, e.g., [certbot](https://certbot.eff.org/). The reverse proxy [Traefik](https://docs.traefik.io/) also supports ACME.

#### `ACME_FILE`

The `mlan/asterisk` image looks for a file `ACME_FILE=/acme/acme.json`. at container startup and every time this file changes certificates within this file are exported and if the host name of one of those certificates matches `HOSTNAME=$(hostname)` is will be used for TLS support.

So reusing certificates from Traefik will work out of the box if the `/acme` directory in the Traefik container is also mounted in the `mlan/asterisk` container.

```bash
docker run -d -v proxy-acme:/acme:ro mlan/asterisk
```

## Security - Intrusion prevention

Attempts by attackers to crack SIP passwords and hijack SIP accounts are very common. Most likely the server will have to fend off thousands of attempts every day. We will mention three means to improve intrusion prevention; obscurity by using non-standard ports, SIP passwords strength, and AutoBan; an Intrusion Detection and Prevention System.

### Obscurity by using non-standard ports

When using non-standard ports the amount of attacks drop significantly, so it might be considered whenever practical. When changing port numbers they need to be updated both for docker and asterisk. To exemplify, assume we want to use 5560 for UDP and TCP and 5561 for TLS, in which case we update the configuration in two places:

- docker/docker-compose, e.g., `docker run -p "5560-5561:5560-5561" -p"5560:5560/udp" ...`
- asterisk transport in `pjsip_transport.conf` (`pjsip.conf`), e.g. `bind = 0.0.0.0:5560` and `bind = 0.0.0.0:5561`

### SIP passwords strength

It’s recommended that the minimum strength of a password used in a SIP digests are at least 8 characters long, preferably 10 characters, and have characters that include lower and upper case alphabetic, a number and a non-alphabetic, non-numeric ASCII character, see [SIP Password Security - How much is yours worth?](https://www.sipsorcery.com/mainsite/Help/SIPPasswordSecurity).

# Container audio

The `mlan/asterisk` container supports two-way audio using [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/). This allows you to use the Asterisk console channel to do some management or debugging. The audio stream is passed between container and host by sharing the user's pulse unix socket.

The method described here was chosen since it allows audio to be enabled on an already running container. The method involves a directory `./pulse:/run/pulse:rshared ` on the host being mounted on the container, see the [compose example](#docker-compose-example), and environment variables being set within the container, allowing pulse to locate the socket; `PULSE_SERVER=unix:/run/pulse/socket` and cookie; `PULSE_COOKIE=/run/pulse/cookie`.

Often we are not interested in sharing audio with the container and the above bind mount and environment variables achieve nothing. But should there come a time when we want to enable the audio, we can do so in 3 simple steps: 1) Mount the user's pulse socket in the host directory `./pulse/socket` and, 2) copy the user's pulse cookie there too, `./pulse/cookie`. 3) Have asterisk, running inside the container, load the `chan_alsa.so` module. From a shell running on the host, these steps are:

```sh
cp -f ${PULSE_COOKIE-$HOME/.config/pulse/cookie} pulse/cookie
sudo mount --bind $(pactl info | sed '1!d;s/.*:\s*//g') pulse/socket
docker-compose exec $(SRV_NAME) asterisk -rx 'module load chan_alsa.so'
```
A limitation of this approach is that you need sudo/root access do be able to bind mount on the host. Naturally, there needs to be a pulse server running on the host for any of this to work.

## Playing with audio

You can use the [demo](#docker-compose-example) above, and once the container is running, enable audio by typing from within the `demo` directory:

```bash
make sound_enable
```

Now you can tryout any of the trivial sound checks, for example:

```bash
make sound_5
```

To disable audio, type:

```bash
make sound_disable
```

# Add-ons

The `mlan/asterisk` repository contains add-ons that utilizes and extends the already impressive capabilities of Asterisk.

## [PrivateDial](src/privatedial/README.md)

PrivateDial is a suite of [Asterisk configuration files](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Configuration+Files). This configuration is tailored to residential use cases, supporting the capabilities of mobile smart phones, that is, voice, video, instant messaging or SMS, and voice mail delivered by email.

It uses the [PJSIP](https://www.pjsip.org/) [channel driver](https://wiki.asterisk.org/wiki/display/AST/Configuring+res_pjsip) and therefore natively support simultaneous connection of several soft-phones to each user account/endpoint.

The underlying design idea is to separate the dial plan functionality from the user data. To achieve this all user specific data has been pushed out from the main `extensions.conf` file.

## [AutoBan](src/autoban/README.md)

AutoBan is an intrusion detection and prevention system which is built in the `mlan/asterisk` container. The intrusion detection is achieved by Asterisk itself. Asterisk generates security events which AutoBan listens to on the AMI interface. When security events occurs AutoBan start to watch the source IP address. Intrusion prevention is achieved by AutoBan asking the Linux kernel firewall [nftables](https://netfilter.org/projects/nftables/) to drop packages from offending source IP addresses.

## [WebSMS](src/websms/README.md)

Asterisk supports [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)), allowing SMS to be sent using the extended SIP method; MESSAGE, natively. Still many [Internet Telephony Service Providers](wikipedia.org/wiki/Internet_telephony_service_provider) (ITSP) does not offer SIMPLE but instead sends and receives SMS using a web [API](https://en.wikipedia.org/wiki/Application_programming_interface) based on [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) requests. This leaves your Asterisk server without a mechanisms to exchange SMS externally.

The WebSMS service bridges this limitation, with the help of two components. One, `websmsd`, waits for incoming SMS to be sent from your ITSP and once received, forward it to Asterisk. The other, `websms`, is used by Asterisk to send outgoing SMS to your ITSP.

#### `WEBSMSD_PORT`

WebSMS uses the PHP integrated web server. The environment variable `WEBSMSD_PORT=80` determinate which port the web server listens to. If `WEBSMSD_PORT` is undefined or non-numeric the PHP web server is disabled and, consequently, WebSMS too. Disabling the web server might be desired in scenarios when the container runs in host mode and there are other services running on the host blocking ports of concern.

# Implementation

TODO!

entrypoint.d

Persistence





Build variables.

| Variable             | Default                    | Description            |
| -------------------- | -------------------------- | ---------------------- |
| DOCKER_ACME_SSL_DIR  | ${DOCKER_SSL_DIR}/acme     |                        |
| DOCKER_AST_SSL_DIR   | ${DOCKER_SSL_DIR}/asterisk |                        |
| DOCKER_BIN_DIR       | /usr/local/bin             |                        |
| DOCKER_CONF_DIR      | /etc/asterisk              |                        |
| DOCKER_ENTRY_DIR     | /etc/entrypoint.d          |                        |
| DOCKER_EXIT_DIR      | /etc/exitpoint.d           |                        |
| DOCKER_LIB_DIR       | /var/lib/asterisk          |                        |
| DOCKER_LOG_DIR       | /var/log/asterisk          |                        |
| DOCKER_MOH_DIR       | ${DOCKER_LIB_DIR}/moh      |                        |
| DOCKER_NFT_DIR       | /var/lib/nftables          |                        |
| DOCKER_NFT_FILE      | autoban.nft                |                        |
| DOCKER_PERSIST_DIR   | /srv                       |                        |
| DOCKER_PHP_DIR       | /usr/share/php7            |                        |
| DOCKER_RUN_DIR       | /var/run                   | Only in setup-runit.sh |
| DOCKER_RUNSV_DIR     | /etc/service               |                        |
| DOCKER_SEED_CONF_DIR | /usr/share/asterisk/config |                        |
| DOCKER_SEED_NFT_DIR  | /etc/nftables              |                        |
| DOCKER_SPOOL_DIR     | /var/spool/asterisk        |                        |
| DOCKER_SSL_DIR       | /etc/ssl                   |                        |

