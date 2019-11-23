# PrivateDial

PrivateDial is an easily customized asterisk configuration. It is tailored to for private use cases, supporting the capabilities of mobile smart phones, that is, voice, video, instant messaging or SMS, and voice mail delivered by email.

It uses the PJSIP channel driver, and therefore natively support simultaneous connection of several soft-phones to each user account/endpoint.

The underlying design idea is to separate the dial plan function form the user data. To achieve this all user specific data has been pushed out from the main file `extensions.conf`.

## Features

Feature list follows below

- PHP web-hook for (incoming) SMS http ISTP origination
- dialplan PHP (outgoing) SMS http ISTP termination
- dialplan ISTP originating (incoming) SIP voice call
- dialplan ISTP termination (outgoing) SIP voice call

## Configuration files

Asterisk and its modules are configured using several configuration files which are typically found in `/etc/asterisk`. The `/mlan/asterisk` image provides a collection of configuration files which can serve as starting point for your system. We will outline how we intend the default configuration files are structured.

### Configuration files overview

| File name               | Description                                                  |
| ----------------------- | ------------------------------------------------------------ |
| extensions.conf  | dialplan how incoming and outgoing calls and messages are handled |
| extensions_local.conf | Defines SIP trunk endpoint                                   |
| minivm.conf             | Define mail sever URL and authentication credentials which voice mail email notifications will be sent |
| pjsip.conf |  |
| pjsip_transport.conf  | Defines SIP transport, protocol, port, host URL              |
| pjsip_wizard.conf       | Defines endpoints, soft-phones, users, sip trunk             |

## Usage

### SIP Trunk

PJSIP endpoints are defined using the [PJSIP Wizard](https://wiki.asterisk.org/wiki/display/AST/PJSIP+Configuration+Wizard) in the configuration file `pjsip_wizard.conf` . For convenience the templates, `tpl_trunk`, `tpl_trunkout` and `tpl_trunkin` has been defined.

Add an endpoint entry in `pjsip_wizard.conf` based on the setup instructions provided by your trunk provider. This entry also hold your authentication credentials.

`pjsip_wizard.conf`

```ini
[trunk_itsp](tpl_trunk)
remote_hosts = sip.itsp.com
outbound_auth/username = username
outbound_auth/password = password
```

`extensions_local.conf`

```ini
[globals]
TRUNK_ENDPOINT = trunk_itsp
```

Most likely you also need to configure WebSMS.

### SIP Users

PJSIP endpoints are defined using the [PJSIP Wizard](https://wiki.asterisk.org/wiki/display/AST/PJSIP+Configuration+Wizard) in the configuration file `pjsip_wizard.conf` . For convenience the template, `tpl_softphone` has been defined.

Add an endpoint entry in `pjsip_wizard.conf` for each user. Each user can simultaneously connect with several soft-phones.

`pjsip_wizard.conf`

```ini
[john.doe](tpl_softphone)
hint_exten = +12025550160
endpoint/callerid = John Doe <+12025550160>
endpoint/mailboxes = john.doe@example.com
endpoint/from_user = +12025550160
inbound_auth/username = john.doe
inbound_auth/password = password
```

### Outgoing SMTP email server

`minivm.conf`

```ini
[general]
mailcmd = minivm-send -H mx.example.com:587 -P starttls -u username -p password -f voicemail-noreply@example.com
```

### Custom SIP ports

`pjsip_transport.conf`

```ini
[tpl_wan](!)
type = transport
bind = 0.0.0.0:5560
...
[tls](tpl_wan)
type = transport
bind = 0.0.0.0:5561
...
```

### TLS Certificate

Copy to container with names:

`pjsip_transport.conf`

```ini
[tls](tpl_wan)
cert_file = /etc/ssl/asterisk/asterisk.cert.pem
priv_key_file = /etc/ssl/asterisk/asterisk.priv_key.pem
```
