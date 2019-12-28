#  WebSMS

The [Short Message Service (SMS)](https://en.wikipedia.org/wiki/SMS) [text messaging](https://en.wikipedia.org/wiki/Text_messaging) service, introduced in 1993, enabled mobile devices to exchange short text messages, using the [Short Message Peer-to-Peer (SMPP)](https://en.wikipedia.org/wiki/Short_Message_Peer-to-Peer) protocol. The [Session Initiation Protocol (SIP)](wikipedia.org/wiki/Session_Initiation_Protocol) include provision for [Instant Messaging (IM)](https://en.wikipedia.org/wiki/Instant_messaging) using the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol extension, serving a similar purpose.

Asterisk supports [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) natively. Still many [Internet Telephony Service Providers](wikipedia.org/wiki/Internet_telephony_service_provider) (ITSP) does not offer SIMPLE but instead sends and receives SMS using a web [API](https://en.wikipedia.org/wiki/Application_programming_interface) based on [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) requests. This leaves Asterisk without a mechanisms to exchange SMS externally.

The WebSMS service bridges this imitation, with the help of two components. One, `websmsd`, waits for incoming SMS to be sent from your ITSP and once received, forward it to Asterisk. The other, `websms`, is used by Asterisk to send outgoing SMS to your ITSP.

## Operation

Asterisk natively handles SMS in between soft-phones using  the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol. When SMS is sent out to your ITSP Asterisk uses an utility, `websms`, to send a HTTP [POST](https://en.wikipedia.org/wiki/POST_(HTTP)) request, containing the extension number, caller id and the message text, to the ITSP web [API](https://en.wikipedia.org/wiki/Application_programming_interface). Normally this request need to be authenticated using credentials provided by the ITSP.

The `websmsd` client listens for HTTP POST request which your ITSP will issue when there is an incoming SMS. The request includes the extension number, caller id and the message text. Once received, this message is placed in the Asterisk call queue, using a [call file](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Call+FIles). Asterisk will pick up the queued message and forward it to the relevant soft-phone using the SIMPLE protocol. For this to work you need to provide your ITSP with the URL to the `websmsd` client.

### Selection of ITSP

Not all ITSP offer [virtual numbers (DID)](https://en.wikipedia.org/wiki/Virtual_number) which can send and receive SMS, in the region you are interested in. So it might be a good idea to spend some time investigating what is available.

### Emoticons and encoding

Modern phones support [Unicode](https://en.wikipedia.org/wiki/Universal_Coded_Character_Set) for non-GSM (GSM-7) characters; Unicode UCS-2. This gives a larger set of characters at a cost in [message length](https://en.wikipedia.org/wiki/SMS#Message_size). To be able to send and receive SMS with emoticons, the ITSP's API needs to support the UCS-2 encoding which is not always the case.

Nowadays most smart phones uses UTF-16 in setad of UCS-2.

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
| [astqueue] | message_encode  | rfc3986                      | string  | Only single line allowed in call file so url-encoding message. |

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

Currently there can only be one WebSMS configuration, so it is not possible to send or receive SMS from more than one ITSP.

### websms.php sending SMS to ITSP

The function of `websms.php` in the SMS data flow is to transfer the message out of Asterisk on to the system of the ITSP. The underlying mechanism for this is a HTTP(S) request executed using [cURL](https://curl.haxx.se/). Admittedly, since Asterisk comes with integrated support for cURL using [libcurl](https://curl.haxx.se/libcurl/) it would be possible to implement the  `websms` functionality without of going the route of calling a PHP script. The main motivation of `websms` is therefore "ease of use" since it can better leverage the companion function `websmsd`.

To describe the data flow we walk trough an example where a soft-phone (endpoint) user sends a SMS to a destination outside of the PBX. The endpoint sends a SIP MESSAGE request [RFC3428](https://tools.ietf.org/html/rfc3428) to Asterisk and a [channel](https://wiki.asterisk.org/wiki/display/AST/Channels) is set up and placed in the dial-plan. The channel variables include the, `EXTEN`, `MESSAGE(to)`, `MESSAGE(from)`, and `MESSAGE(body)`. The external destination is identified in the dial-plan and `websms.php` is call via [Asterisk Gateway Interface (AGI)](https://wiki.asterisk.org/wiki/pages/viewpage.action?pageId=32375589) in the dial-plan (extensions.conf):

```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${QUOTE(${MESSAGE(body)})})
```

The `MESSAGE(body)` needs to be quoted since it can include special characters. With the provided arguments `websms.php` sends an authenticated HTTP request to the API of ITSP who will continue forward the SMS to its final destination.

The payload of the HTTP request might look like this:

```json
{"to":"+12025550183","from":"+12025550160","body":"Outgoing message!"}
```

For testing purposes, you can use `websms.php` to send a SMS from the command line inside the container:

```bash
/usr/share/php7/websms.php +12025550183 +12025550160 "Outgoing message!"
```

### websmsd.php receiving SMS from ITSP

This PHP script listens to HTTP requests, representing incoming SMS,
from your ITSP and generate call files which will be picked up by asterisk.

Run with the PHP built-in web server:

```bash
php -S 0.0.0.0:80 /path/websmsd.php
```
The ITSP receives a SMS addressed to your virtual number, so the API sends a HTTP request to `websmsd.php` with the following payload.

```json
{"to":"+12025550160","from":"+15017122661","body":"Incoming message!"}
```

With the payload received we need to forward the SMS data to Asterisk so it can send to the endpoint.

The method to 

#### Call files

[Call files](http://the-asterisk-book.com/1.6/call-file.html) are like a shell script for Asterisk. A user or application writes a call file into `/var/spool/asterisk/outgoing/` where Asterisk processes it immediately.

The message cannot span multiple lines in an Asterisk call file. To work
around that we encode the message ([RFC3986](https://tools.ietf.org/html/rfc3986), which obsolete [RFC2396](https://tools.ietf.org/html/rfc2396)).

This is an example [call file](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Call+FIles) with encoded MESSAGE(body).
```ini
Channel: Local/+12025550160@dp_entry_channel_open
CallerID: "" <+15017122661>
WaitTime: 45
MaxRetries: 0
RetryTime: 300
Context: dp_entry_trunk_texting
Extension: +12025550160
Priority: 1
Archive: yes
setvar: MESSAGE(to)=+12025550160
setvar: MESSAGE(from)=+15017122661
setvar: MESSAGE(body)=Incoming%20message%21.
setvar: MESSAGE_ENCODE=rfc3986
```

To make sure Asterisk does not tries to read the call file before it is fully written...

Asterisk uses the function [`MESSAGE()`](https://wiki.asterisk.org/wiki/display/AST/Asterisk+17+Function_MESSAGE) to access the SMS data.

#### Outline

Define error handler and load variable values.

Respond to echo requests.

Read the post header data.

Generate call file name.

Create new call file in the staging directory.

Move the call file to the outgoing directory, so that Asterisk pick it up.

Respond with a status message.
