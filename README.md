# The `mlan/asterisk` repository

![travis-ci test](https://img.shields.io/travis/mlan/docker-asterisk.svg?label=build&style=popout-square&logo=travis)
![docker build](https://img.shields.io/docker/cloud/build/mlan/asterisk.svg?label=build&style=popout-square&logo=docker)
![image size](https://img.shields.io/docker/image-size/mlan/asterisk.svg?label=size&style=popout-square&logo=docker)
![docker stars](https://img.shields.io/docker/stars/mlan/asterisk.svg?label=stars&style=popout-square&logo=docker)
![docker pulls](https://img.shields.io/docker/pulls/mlan/asterisk.svg?label=pulls&style=popout-square&logo=docker)

THIS DOCUMENT IS UNDER DEVELOPMENT.

This (non official) repository provides dockerized PBX.

## Features

Feature list follows below

- Asterisk PBX
- [PrivateDial](src/privatedial/README.md), an easily customized asterisk configuration
- [WebSMS](srs/websms/README.md), send and receive Instant Messages, SMS over HTTP
- [AutoBan](src/autoban/README.md), a built in intrusion detection and prevention system
- Alpine Linux

## Tags

The breaking.feature.fix [semantic versioning](https://semver.org/)
used. In addition to the three number version number you can use two or
one number versions numbers, which refers to the latest version of the 
sub series. The tag `latest` references the build based on the latest commit to the repository.

The `mlan/asterisk` repository contains a multi staged built. You select which build using the appropriate tag from `mini`, `base`, `full` and `xtra`. The image `mini` only contain Asterisk.
To exemplify the usage of the tags, lets assume that the latest version is `1.0.0`. In this case `latest`, `1.0.0`, `1.0`, `1`, `full`, `full-1.0.0`, `full-1.0` and `full-1` all identify the same image.

# Usage

Often you want to configure Asterisk and its components. There are different methods available to achieve this. Moreover docker volumes or host directories with desired configuration files can be mounted in the container. And finally you can `docker exec` into a running container and modify configuration files directly.

If you want to test the image right away, probably the best way is to clone the github repository and run the demo therein.

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
    cap_add:
      - net_admin
      - net_raw
    ports:
      - "${SMS_PORT-8080}:80"
      - "9960:9960/udp"
      - "9960:9960"
      - "9961:9961"
      - "10000-10099:10000-10099/udp"
    environment:
      - SYSLOG_LEVEL=${SYSLOG_LEVEL-4}
      - HOSTNAME=${TELE_SRV-tele}.${DOMAIN-docker.localhost}
    volumes:
      - tele-conf:/srv

volumes:
  tele-conf:
```

This repository contains a `demo` directory which hold the `docker-compose.yml` file as well as a `Makefile` which might come handy. From within the `demo` directory you can start the container simply by typing:

```bash
make up
```

The you can connect to the asterisk command line interface (CLI) running inside the container by typing

```bash
make cli
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

FIX THIS SECTION!

When you create the `mlan/asterisk` container, you can configure the services by passing one or more environment variables or arguments on the docker run command line. Once the services has been configured a lock file is created, to avoid repeating the configuration procedure when the container is restated. In the rare event that want to modify the configuration of an existing container you can override the default behavior by setting `FORCE_CONFIG` to a no-empty string.

| Variable                                   | Default         | Description                                   |
| ------------------------------------------ | --------------- | --------------------------------------------- |
| ACME_FILE                                  | /acme/acme.json |                                               |
| ASTERISK_SMSD_DEBUG                        |                 |                                               |
| HOSTNAME                                   | $(hostname)     |                                               |
| SYSLOG_LEVEL                               | 4               |                                               |
| SYSLOG_OPTIONS                             | -SDt            |                                               |
| [TLS_CERTDAYS](#tls_keybits-tls_certdays)  | 30              | Self-signed TLS certificate validity duration |
| [TLS_KEYBITS](#tls_keybits-tls_certdays)   | 2048            | Self-signed TLS key length                    |

## Configuration files

Asterisk and its modules are configured using several configuration files which are typically found in `/etc/asterisk`. The `/mlan/asterisk` image provides a collection of configuration files which can serve as starting point for your system. We will outline how we intend the default configuration files are structured.

### Configuration files overview

Some of the collection of configuration files provided does not contain any user specific data and might initially be left unmodified. These files are:

| File name        | Description                                |
| ---------------- | ------------------------------------------ |
| alsa.conf        |                                            |
| asterisk.conf    | asterisk logging, directory structure      |
| ccss.conf        |                                            |
| cli_aliases.conf | command line interface aliases convenience |
| features.conf    | activation of special features             |
| indications.conf | dial tone local                            |
| logger.conf      | logfiles                                   |
| modules.conf     | activation of modules                      |
| musiconhold.conf | music on hold directory                    |
| pjproject.conf   | pjsip installation version                 |
| rtp.conf         | Define RTP port range                      |

## Persistent storage

By default, docker will store the configuration and run data within the container. This has the drawback that the configurations and queued and quarantined mail are lost together with the container should it be deleted. It can therefore be a good idea to use docker volumes and mount the run directories and/or the configuration directories there so that the data will survive a container deletion.

To facilitate such approach, to achieve persistent storage, the configuration and run directories of the services has been consolidated to `/srv/etc` and `/srv/var` respectively. So if you to have chosen to use both persistent configuration and run data you can run the container like this:

```
docker run -d --name pbx-mta -v pbx-mta:/srv -p 127.0.0.1:25:25 mlan/asterisk
```

## Initialization procedure

The `mlan/asterisk` image is compiled without any configuration files. When a container is created using the `mlan/asterisk` image default configuration files are copied to the configuration directory `etc/asterisk` if it is found to be empty. This behavior is intended to support the following initialization procedures.

In scenarios where you already have a collection of configuration files on a docker volume, start/create a `mlan/asterisk` container with this volume mounted. At startup these configuration files are recognized and left untouched and asterisk is stated. The same will happen when the container is restarted. 

In a scenario where we don't have any configuration files yet we start/create a want to start `mlan/asterisk` container with an empty target volume. At startup the default configuration files will be copied to the mounted volume. Now you can edit these configuration files to your liking either from within the container or directly from the volume mounting point on the docker host. At consecutive startup these configuration files are recognized and left untouched and asterisk is stated.

## Logging `SYSLOG_LEVEL`

The level of output for logging is in the range from 0 to 8. 1 means emergency logging only, 2 for alert messages, 3 for critical messages only, 4 for error or worse, 5 for warning or worse, 6 for notice or worse, 7 for info or worse, 8 debug. Default: `SYSLOG_LEVEL=4`

# Networking
SIP networking is quite complex and there is many things that can go wrong. We try to offer some guidance by discussing some fundamentals here.

## SIP protocol

The [Session Initiation Protocol (SIP)](https://en.wikipedia.org/wiki/Session_Initiation_Protocol) is a [signaling protocol](https://en.wikipedia.org/wiki/Signaling_protocol) used for initiating, maintaining, and terminating real-time sessions that include voice, video and messaging applications.

### Transports, UDP, TCP and TLS

SIP is designed to be independent of the underlying [transport layer](https://en.wikipedia.org/wiki/Transport_layer) protocol, and can be used with the [User Datagram Protocol](https://en.wikipedia.org/wiki/User_Datagram_Protocol) (UDP), the [Transmission Control Protocol](https://en.wikipedia.org/wiki/Transmission_Control_Protocol) (TCP), and the [Stream Control Transmission Protocol](https://en.wikipedia.org/wiki/Stream_Control_Transmission_Protocol) (SCTP). For secure transmissions of SIP messages over insecure network links, the protocol may be encrypted with [Transport Layer Security](https://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS). For the transmission of media streams (voice, video) the [Session Description Protocol](https://en.wikipedia.org/wiki/Session_Description_Protocol) (SDP) payload carried in SIP messages typically employs the [Real-time Transport Protocol](https://en.wikipedia.org/wiki/Real-time_Transport_Protocol) (RTP) or the [Secure Real-time Transport Protocol](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol) (SRTP).

TLS protects against attackers who try to listen on the signaling link but it does not provide end-to-end security to prevent espionage and law enforcement interception, as the encryption is only hop-by-hop and every single intermediate proxy has to be trusted. The media streams which are separate connections from the signaling stream, may be encrypted with the [Secure Real-time Transport Protocol](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol) (SRTP). The key exchange for SRTP is performed with [SDES](https://en.wikipedia.org/wiki/SDES), or with [ZRTP](https://en.wikipedia.org/wiki/ZRTP).

### Ports, 5060, 5061 and 10000-20000

SIP clients typically use TCP or UDP on [port numbers](https://en.wikipedia.org/wiki/Port_number) 5060 or 5061 for SIP traffic to servers and other endpoints. Port 5060 is commonly used for non-encrypted signaling traffic whereas port 5061 is typically used for traffic encrypted with [Transport Layer Security](https://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS).

RTP uses a dynamic port range (and is only ever UDP), generally between 10000-20000. This range can usually be customized on the client to suit differing firewall configurations or other concerns.

## Docker, iptables and docker-proxy

When publishing ports on a docker container, using default `bridge` networking, two things happen; routing rules are updated in the Linux kernel firewall and a proxy processes are stated. The current networking implementation in docker (19.03.5), built on the aged [iptables](https://en.wikipedia.org/wiki/Iptables) and docker-proxy, cannot publish port ranges, but instead publish them one by one. This is problematic since normally RTP uses the port range 10000-20000 and updating the firewall 10001 times and staring 10001 proxy processes can take unacceptable long time (minutes) and often prevents the container to be stated all together. Long term this limitation will be addressed since it is inevitable that docker networking will have to be redesigned. But right now we have to work around this imitation and we describe two ways to address this here.

First, you can use the `host` network mode (`docker run --network host ...`). The host network mode does not use the docker-proxy and does not need to set up routing rules in the firewall. The downside is that a container stated in this way cannot communicate on any of the docker networks.

Second, you can stay with the default or user-defined `bridge` mode and instead limit the RTP port range to something manageable say 10000-10099 up to 10000-10999. This actually seems to work in practice, at least with some trunk providers (ITSP). The RTP port range is configured in `rtp.conf` `rtpstart = 10000, rtpend = 10099`.

## Network address translation (NAT)

[Network address translation (NAT)](https://en.wikipedia.org/wiki/Network_address_translation) is a method of remapping one IP [address space](https://en.wikipedia.org/wiki/Address_space) into another by modifying [network address](https://en.wikipedia.org/wiki/Network_address) information in the [IP header](https://en.wikipedia.org/wiki/IP_header) of packets while they are in transit across a traffic [routing device](https://en.wikipedia.org/wiki/Router_(computing)). Here two network environments often results in NAT being used. First, the SIP server we deploy using  `mlan/asterisk` runs inside a docker container and depending on what type of network we choose docker will start a proxy process.

### Sip host and domain name

The host name need to be set in three files:

- docker/docker-compose, e.g., `docker run -e HOSTNAME=sip.example.com ...`
- `pjsip-local.conf` `domain = sip.example.com`, `external_media_address = sip.example.com`, and `external_signaling_address = sip.example.com`
- `pjsip_wizard.conf` `endpoint/from_domain = sip.example.com`

With these settings you should not need ICE, STUN or TURN.

## Security - Privacy and integrity

[Transport Layer Security](http://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS) provides encryption for call signaling. A excellent guide for setting up TLS between Asterisk and a SIP client, involving creating key files, modifying Asterisk's SIP configuration to enable TLS, creating a SIP endpoint/user that's capable of TLS, and modifying the SIP client to connect  to Asterisk over TLS, can be found here [Secure Calling Tutorial](https://wiki.asterisk.org/wiki/display/AST/Secure+Calling+Tutorial). 

The PrivateDial configuration is already set up to provide both UDP and TCP. TLS and SDES SRTP are also prepared, but a [TLS/SSL server certificate](https://en.wikipedia.org/wiki/Public_key_certificate) and key are needed for their activation. If the certificate and key do not exist when the container starts a [self-signed certificate](https://en.wikipedia.org/wiki/Self-signed_certificate) and private key will automatically be generated.

####  `TLS_KEYBITS`, `TLS_CERTDAYS`

 `TLS_KEYBITS=2048`, `TLS_CERTDAYS=30`.

There is also a mechanism to use ACME lets encrypt certificates.

### Let’s Encrypt LTS certificates using Traefik

Let’s Encrypt provide free, automated, authorized certificates when you can demonstrate control over your domain. Automatic Certificate Management Environment (ACME) is the protocol used for such demonstration. There are many agents and applications that supports ACME, e.g., [certbot](https://certbot.eff.org/). The reverse proxy [Traefik](https://docs.traefik.io/) also supports ACME.

#### `ACME_FILE`

The `mlan/amavis` image looks for a file `ACME_FILE=/acme/acme.json`. at container startup and every time this file changes certificates within this file are exported and if the host name of one of those certificates matches `HOSTNAME=$(hostname)` is will be used for TLS support.

So reusing certificates from Traefik will work out of the box if the `/acme` directory in the Traefik container is also mounted in the `mlan/asterisk` container.

```bash
docker run -d -v proxy-acme:/acme:ro mlan/asterisk
```

## Security - Intrusion prevention

Attempts by attackers to crack SIP passwords and hijack SIP accounts are very common. Most likely the server will have to fend off thousands of attempts every day. We will mention three means to improve intrusion prevention; obscurity by using non-standard ports, SIP passwords strength, and AutoBan; an Intrusion Detection and Prevention System.

### Obscurity by using non-standard ports

When using non-standard ports the amount of attacks drop significantly, so it might be considered whenever practical. When changing port numbers they need to be updated both for docker and asterisk. To exemplify, assume we want to use 5560 for UDP and TCP and 5561 for TLS, in which case we update the configuration in two places:

- docker/docker-compose, eg, `docker run -p "5560-5561:5560-5561" -p"5560:5560/udp" ...`
- asterisk transport in `pjsip_transport.conf` (`pjsip.conf`), eg `bind = 0.0.0.0:5560` and `bind = 0.0.0.0:5561`

### SIP passwords strength

It’s recommended that the minimum strength of a password used in a SIP digests are at least 8 characters long, preferably 10 characters, and have characters that include lower and upper case alphabetic, a number and a non-alphabetic, non-numeric ASCII character, see [SIP Password Security - How much is yours worth?](https://www.sipsorcery.com/mainsite/Help/SIPPasswordSecurity).

# Console PulseAudio

`/etc/pulse/daemon.conf`

```ini
default-fragments = 5
default-fragment-size-msec = 1
```

```bash
pulseaudio -k
```

`/usr/share/applications/asterisk.desktop`

```ini
[Desktop Entry]
Name=Asterisk
Type=Application
Categories=Telephony
X-PulseAudio-Properties=media.role=phone
```

# Add-ons

The `mlan/asterisk` repository contains add-ons that utilizes and extends the already impressive capabilities of Asterisk.

## PrivateDial

[PrivateDial](src/privatedial/README.md), an easily customized asterisk configuration. It is described [here](src/privatedial/doc/privatedial.md).

## AutoBan

[AutoBan](src/autoban/README.md) is an intrusion detection and prevention system which is built in the `mlan/asterisk` container. It is described [here](src/autoban/doc/autoban.md).

## WebSMS

The [websms](src/websms/README.md) service is described [here](src/websms/doc/websms.md).

# Missing

TCP provides much more stable SIP functionality than UDP.

## Implementation

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

