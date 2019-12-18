#  WebSMS

The [Short Message Service (SMS)](https://en.wikipedia.org/wiki/SMS) [text messaging](https://en.wikipedia.org/wiki/Text_messaging) service, introduced in 1993, enabled mobile devices to exchange short text messages, using the [Global System for Mobile Communications (GSM)](https://en.wikipedia.org/wiki/GSM) network. The [Session Initiation Protocol (SIP)](wikipedia.org/wiki/Session_Initiation_Protocol) include provision for [Instant Messaging (IM)](https://en.wikipedia.org/wiki/Instant_messaging) using the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol extension, serving a similar purpose.

Asterisk supports [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) natively. Still many [Internet Telephony Service Providers](wikipedia.org/wiki/Internet_telephony_service_provider) (ITSP) does not offer SIMPLE but instead sends and receives SMS using a web [API](https://en.wikipedia.org/wiki/Application_programming_interface) based on [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) requests. This leaves Asterisk without a mechanisms to exchange SMS externally.

The WebSMS service bridges this imitation, with the help of two components. One, `websmsd`, waits for incoming SMS to be sent from your ITSP and once received, forward it to Asterisk. The other, `websms`, is used by Asterisk to send outgoing SMS to your ITSP.

## Operation

Asterisk natively handles SMS in between soft-phones using  the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol. When SMS is sent out to your ITSP Asterisk uses an utility, `websms`, to send a HTTP [POST](https://en.wikipedia.org/wiki/POST_(HTTP)) request, containing the extension number, caller id and the message text, to the ITSP web [API](https://en.wikipedia.org/wiki/Application_programming_interface). Normally this request need to be authenticated using credentials provided by the ITSP.

The `websmsd` client listens for HTTP POST request which your ITSP will issue when there is an incoming SMS. The request includes the extension number, caller id and the message text. Once received, this message is placed in the Asterisk call queue, using a [call file](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Call+FIles). Asterisk will pick up the queued message and forward it to the relevant soft-phone using the SIMPLE protocol. For this to work you need to provide your ITSP with the URL to the `websmsd` client.

### Selection of ITSP

Not all ITSP offer [virtual numbers (DID)](https://en.wikipedia.org/wiki/Virtual_number) which can send and receive SMS, in the region you are interested in. So it might be a good idea to spend some time investigating what is available.

### Emoticons and encoding

Modern phones support [Unicode](https://en.wikipedia.org/wiki/Universal_Coded_Character_Set) for non-GSM (GSM-7) characters; UCS-2. This gives a larger set of characters at a cost in [message length](https://en.wikipedia.org/wiki/SMS#Message_size). To be able to send and receive SMS with emoticons, the ITSP's API needs to support the UCS-2 encoding which is not always the case.

### Reverse proxy

I some scenarios it can be beneficial to use a [reverse proxy](https://en.wikipedia.org/wiki/Reverse_proxy) like [traefik](https://containo.us/traefik/), also providing [HTTPS](https://en.wikipedia.org/wiki/HTTPS), to route the HTTP(S) requests to the `websmsd` client.

## Configuration

The function of WebSMS is controlled by the configuration file; `websms.conf` and the configuration in your account with the ITSP. The configuration file has three sections, they are: `[websms]` configuring outgoing SMS to the ITSP, `[websmsd]` configuring incoming SMS from the ITSP, and `[astqueue]` configuring the call queue, using call files, where incoming SMS are placed so that Asterisk can pick them up.


| File name   | Description                                            |
| ----------- | ------------------------------------------------------ |
| websms.conf | Configurations which are unique to the WebSMS services |
### HTTP request header keys

ITSP has implemented their SMS API a little differently. Study the ITSP documentation and configure the `key_to`, `key_from` and `key_body` appropriately.

```ini
key_to          = To
key_from        = From
key_body        = Body
```

### Outgoing authentication method

Most ITSP requires `websms` to authenticate when sending outgoing SMS to their API. When you set up an account with the ITSP they will provide you with the appropriate user/id and password/secret.

```ini
[websms]
host            = https://api.example.com
path            = /sms/send/
auth_user       = id
auth_passwd     = secret
auth_method     = basic
```

Not all ITSP use the same authentication method.
Currently there is support for: `basic` and `zadarma`.

#### `basic`

[Basic access authentication](wikipedia.org/wiki/Basic_access_authentication),
is a method for an [HTTP user agent](https://en.wikipedia.org/wiki/User_agent) (here `websms`) to provide a [user name](https://en.wikipedia.org/wiki/User_name) and [password](https://en.wikipedia.org/wiki/Password) when making a request. In basic HTTP authentication, a request contains a header field in the form of `Authorization: Basic <credentials>`, where credentials is the [base64](https://en.wikipedia.org/wiki/Base64) encoding of id and password joined by a  single colon `:`.

When using the `basic` authentication method, it is not important how the full URL is separated into  `host` and `path`.

#### `zadarma`

The ITSP [Zadarma](zadarma.com/en/support/api) uses an authentication method using a signature, `<signature>`, computed using the actual message and a secret key, to provide additional security. The request will use a header like: `Authorization: <user_key>:<signature>`.

The signature also uses the `path` part of the full request URL. To accommodate this scheme the full URL is separated into `host` and `path`. The actual request will use a URL which is the concatenation of `host` and `path`.

### Incoming access control

Since most ITSP does not implement incoming authentication, but operate using a limited range of IP addresses, we can filter incoming source addresses to achieve some access control. Use `permit_addr` to limit incoming access using comma separated address ranges in [CIDR](https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing) format. By default any source addresses is permitted.

When `websms` operates behind a reverse proxy we need to trust that the proxy reports the original source addresses. Use `proxy_addr` to indicate the addresses of your trusted proxies using comma separated address ranges in [CIDR](https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing) format. Often proxies sends the original source address in the header `HTTP_X_FORWARDED_FOR`.

```ini
[websmsd]
permit_addr     = 185.45.152.42,3.104.90.0/24,3.1.77.0/24
proxy_addr      = 172.16.0.0/12
proxy_header    = HTTP_X_FORWARDED_FOR
```

### Quirks

`response_check`

`number_format`

`charset`

`key_echo`

`report_success`



### Call queue contexts

When using the PrivateDial dial-plan (extensions.conf), which has integrated the WebSMS service, the proper contexts are:

```ini
[astqueue]
channel_context = dp_entry_channel_open
context         = dp_entry_trunk_texting
```

### Configuring WebSMS, websms.conf

The WebSMS configuration is kept in `websms.conf`. This file is parsed by [PHP](https://secure.php.net/manual/en/function.parse-ini-file.php), which luckily, accepts a syntax similar to Asterisk's configuration files.
One difference is that the strings, "yes", "no", "true", "false" and "null" have to be within quotation marks otherwise they will be interpreted as Boolean by the PHP parser.

| Section    | Key             | Default                      | Format  | Description                                                  |
| ---------- | --------------- | ---------------------------- | ------- | ------------------------------------------------------------ |
| [websms]   | host            |                              | URI     | First half of the URI (Protocol and hostname) of the ITSP API to send SMS to. |
| [websms]   | path            |                              | URI     | Second half of the URI (path).                               |
| [websms]   | key_to          | To                           | string  | HTTP POST key name holding SMS destination phone number      |
| [websms]   | key_from        | From                         | string  | HTTP POST key name holding SMS originating phone number.     |
| [websms]   | key_body        | Body                         | string  | HTTP POST key name holding the SMS message.                  |
| [websms]   | auth_user       |                              | string  | Authentication user/id.                                      |
| [websms]   | auth_passwd     |                              | string  | Authentication password/secret.                              |
| [websms]   | auth_method     | basic                        | string  | Authentication method to use.                                |
| [websms]   | response_check  |                              | string  | HTTP POST key=value to check, eg "status=success".           |
| [websms]   | number_format   |                              | string  | Number format to use, eg "omit+" will omit the leading "+" in international numbers. |
| [websms]   | charset         |                              | string  | Set to "UCS-2" to limit Unicode characters to U+FFFF.        |
| [websmsd]  | key_to          | To                           | string  | HTTP POST key name holding SMS destination phone number.     |
| [websmsd]  | key_from        | From                         | string  | HTTP POST key name holding SMS origination phone number.     |
| [websmsd]  | key_body        | Body                         | string  | HTTP POST key name holding the SMS message.                  |
| [websmsd]  | key_echo        |                              | string  | Some ITSP test that the client respond by expecting it echoing the value in this key, eg "zd_echo". |
| [websmsd]  | key_account     |                              | string  | NOT USED                                                     |
| [websmsd]  | report_success  |                              | string  | Report success like this, eg, "<Response></Response>".       |
| [websmsd]  | permit_addr     |                              | CIDR    | If defined, only listed addresses are permitted, eg 185.45.152.42,3.104.90.0/24,3.1.77.0/24. |
| [websmsd]  | proxy_addr      | 172.16.0.0/12                | CIDR    | Trust "proxy_header" from these IPs, eg 10.0.0.0/8,172.16.0.0/12,192.168.0.0/16. |
| [websmsd]  | proxy_header    | HTTP_X_FORWARDED_FOR         | string  | Behind a proxy this header hold the original client address. |
| [astqueue] | outgoingdir     | /var/spool/asterisk/outgoing | string  | Directory where asterisk picks up call files.                |
| [astqueue] | stagingdir      | /var/spool/asterisk/staging  | string  | Create call file here and then move to outgoing.             |
| [astqueue] | waittime        | 45                           | integer | How many seconds to wait for an answer before the call fails. |
| [astqueue] | maxretries      | 0                            | integer | Number of retries before failing. 0 = don't retry if fails.  |
| [astqueue] | retrytime       | 300                          | integer | How many seconds to wait before retry.                       |
| [astqueue] | archive         | no                           | string  | Use "yes" to save call file to /var/spool/asterisk/outgoing_done |
| [astqueue] | channel_context | default                      | string  | Dialplan context to answer the call, ie set up the channel.  |
| [astqueue] | context         | default                      | string  | Dialplan context to handle the SMS.                          |
| [astqueue] | priority        | 1                            | integer | Dialplan priority to handle the SMS.                         |

### Default configuration

If the Asterisk configuration directory is empty, default configuration files will be copied there at container startup. The one relevant here is `websms.conf`.


```ini
[websms]
host            = api.example.com
path            = /sms/send/
auth_user       = user
auth_passwd     = passwd

[websmsd]

[astqueue]
channel_context = dp_entry_channel_open
context         = dp_entry_trunk_texting
```

## Implementation

implementing a PHP client script, which sends HTTP SMS requests, and a server that listens for HTTP POST request form your ITSP.

### websms

The websms PHP script takes command line arguments and generates a (curl) HTTP
request to the ITSP web API which will send SMS.

Call via [Asterisk Gateway Interface (AGI)](https://wiki.asterisk.org/wiki/pages/viewpage.action?pageId=32375589) in the dial-plan (extensions.conf):
```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${MESSAGE(body)})
```
### websmsd
This PHP script listens to HTTP requests, representing incoming SMS,
from your ITSP and generate call files which will be picked up by asterisk.

Run with the PHP built-in web server:
```bash
php -S 0.0.0.0:80 /path/websmsd.php
```
#### Outline

Define error handler and load variable values.

Respond to echo requests.

Read the post header data.

Generate call file name.

Create new call file in the staging directory.

Move the call file to the outgoing directory, so that Asterisk pick it up.

Respond with a status message.
