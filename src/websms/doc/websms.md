# WebSMS

The [Short Message Service (SMS)](https://en.wikipedia.org/wiki/SMS) [text messaging](https://en.wikipedia.org/wiki/Text_messaging) service, introduced in 1993, enabled mobile devices to exchange short text messages, using the [Short Message Peer-to-Peer (SMPP)](https://en.wikipedia.org/wiki/Short_Message_Peer-to-Peer) protocol. The [Session Initiation Protocol (SIP)](wikipedia.org/wiki/Session_Initiation_Protocol) include provision for [Instant Messaging (IM)](https://en.wikipedia.org/wiki/Instant_messaging) using the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol extension, serving a similar purpose.

Asterisk supports [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) natively. Still many [Internet Telephony Service Providers](wikipedia.org/wiki/Internet_telephony_service_provider) (ITSP) does not offer SIMPLE but instead sends and receives SMS using a web [API](https://en.wikipedia.org/wiki/Application_programming_interface) based on [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) requests. This leaves Asterisk without a mechanisms to exchange SMS externally.

The WebSMS service bridges this limitation, with the help of two components. One, `websmsd`, waits for incoming SMS to be sent from your ITSP and once received, forward it to Asterisk. The other, `websms`, is used by Asterisk to send outgoing SMS to your ITSP.

## Operation

Asterisk natively handles SMS in between soft-phones using the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol. When SMS is sent out to your ITSP Asterisk it uses an utility, `websms`, to send a HTTP [POST](https://en.wikipedia.org/wiki/POST_(HTTP)) request, containing the extension number, caller id and the message text, to the ITSP web [API](https://en.wikipedia.org/wiki/Application_programming_interface). Normally this request need to be authenticated using credentials provided by the ITSP.

The `websmsd` client listens for HTTP POST request which your ITSP will issue when there is an incoming SMS. The request includes the extension number, caller id and the message text. Once received, this message is placed in the Asterisk call queue, using a [call file](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Call+FIles). Asterisk will pick up the queued message and forward it to the relevant soft-phone using the SIMPLE protocol. For this to work you need to provide your ITSP with the URL to the `websmsd` client.

### Selection of ITSP

Not all ITSP offer [virtual numbers (DID)](https://en.wikipedia.org/wiki/Virtual_number) which can send and receive SMS, in the region you are interested in. So it might be a good idea to spend some time investigating what is available.

### Emoticons and encoding

Modern phones support [Unicode](https://en.wikipedia.org/wiki/Universal_Coded_Character_Set) for non-GSM (GSM-7) characters; Historically this was the Unicode UCS-2, but modern systems use UTF-16, which in addition supports 4 byte characters, that is emoticons. Since the maximum SMS message byte length is fixed, Unicode provides a larger set of characters at a cost in [message length](https://en.wikipedia.org/wiki/SMS#Message_size).

Nowadays most smart phones uses UTF-16 encoding in stead of UCS-2. Consequently, to be able to send and receive SMS with all types of emoticons, the ITSP's API needs to support the UTF-16 encoding which is not always the case.

### Reverse proxy

I some scenarios it can be beneficial to use a [reverse proxy](https://en.wikipedia.org/wiki/Reverse_proxy) like [traefik](https://containo.us/traefik/), which is also providing [HTTPS](https://en.wikipedia.org/wiki/HTTPS), to route the HTTP(S) requests to the `websmsd` client.

## Configuration

Some functions of WebSMS are configurable by using a configuration file; `websms.conf`. Typically this file need to include the details of your account with the ITSP. The configuration file has three sections, they are: `[websms]` configuring outgoing SMS to the ITSP, `[websmsd]` configuring incoming SMS from the ITSP, and `[astqueue]` configuring the call queue, using call files, where incoming SMS are placed so that Asterisk can pick them up.

| File name   | Description                                            |
| ----------- | ------------------------------------------------------ |
| websms.conf | Configurations which are unique to the WebSMS services |
### HTTP request header keys

ITSPs have implemented their SMS API a little differently. Study the ITSP documentation and configure the `key_to`, `key_from` and `key_body` appropriately.

```ini
key_to          = To
key_from        = From
key_body        = Body
```

### Outgoing authentication method

Most ITSP requires `websms` to authenticate when sending outgoing SMS via their API. When you set up an account with the ITSP they will provide you with the appropriate user/id and password/secret.

```ini
[websms]
url_host        = https://api.example.com
url_path        = /sms/send/
auth_user       = id
auth_secret     = secret
auth_method     = basic
```

Not all ITSP use the same authentication method.
Currently there is support for: `none`, `plain`, `basic` and `zadarma`.

#### `none`

The POST request is sent without any authentication data.

#### `plain`

The `plain` method uses the parameters `key_user` and `key_secret` in addition to `auth_user` and `auth_secret`.
These are used to add the key-value pairs `<key_user>:<auth_user>` and `<key_secret>:<auth_secret>` to the POST request.

#### `basic`

[Basic access authentication](wikipedia.org/wiki/Basic_access_authentication),
is a method for an [HTTP user agent](https://en.wikipedia.org/wiki/User_agent) (here `websms`) to provide a [user name](https://en.wikipedia.org/wiki/User_name) and [password](https://en.wikipedia.org/wiki/Password) when making a request. In basic HTTP authentication, a request contains a header field in the form of `Authorization: Basic <credentials>`, where credentials is the [base64](https://en.wikipedia.org/wiki/Base64) encoding of id and password joined by a single colon `:`.

When using the `basic` authentication method, it is not important how the full URL is separated into `url_host` and `url_path`.

#### `zadarma`

The ITSP [Zadarma](zadarma.com/en/support/api) uses an authentication method using a signature, `<signature>`, computed using the actual message and a secret key, to provide additional security. The request will use a header like: `Authorization: <user_key>:<signature>`.

The signature also uses the `url_path` part of the full request URL. To accommodate this scheme the full URL is separated into `url_host` and `url_path`. The actual request will use a URL which is the concatenation of `url_host` and `url_path`.

### Incoming access control

Since most ITSP does not implement incoming authentication, but operate using a limited range of IP addresses, we can filter incoming source addresses to achieve some access control. Use `remt_addr` to limit incoming access using comma separated address ranges in [CIDR](https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing) format. By default any source addresses is permitted.

When `websms` operates behind a reverse proxy we need to trust that the proxy reports the original source addresses. Use `prox_addr` to indicate the addresses of your trusted proxies using comma separated address ranges in [CIDR](https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing) format. Often proxies sends the original source address in the header `HTTP_X_FORWARDED_FOR`.

```ini
[websmsd]
remt_addr      = 185.45.152.42,3.104.90.0/24,3.1.77.0/24
prox_addr      = 172.16.0.0/12
prox_header    = HTTP_X_FORWARDED_FOR
```

### Quirks

Despite the API of different ITSP all serve a similar purpose, they all differ somewhat. To allow for this the WebSMS behavior can be modified to accommodate some of such peculiarities.

#### Outgoing response check

Some API respond with a status message to the HTTP request that we send, which can be used to check if the message was sent successfully. We can configure WebSMS to check if the response include the expected "key=value" pair. For example; `resp_check = "status=success"`

#### Outgoing number format

While most API accept any number format, some don't. We can omit the leading "+" in international numbers, by defining `val_numform = "omit+"`.

#### Outgoing character encoding

Many API accepts UTF-16 character encoding, but some do not. In case the API only support UCS-2, it might be required to force WebSMS to use it and thereby limit Unicode character range to `U+FFFF`. This is achieved by defining `val_charset = UCS-2`.

#### Incoming echo

Some API test that it can access your WebSMS web server, by sending a special HTTP request and expecting the response to echo a key value. Configure such echo response by defining the HTTP request key used by the API. For example `key_echo = "zd_echo"`.

#### Incoming response

Some API expects a specific response when sending a HTTP request to know that the transfer was successful. The response to incoming SMS is configured by using `resp_ack`. For example ` resp_ack = "<Response></Response>"`.

### Call queue contexts

When using the PrivateDial dial-plan (extensions.conf), which has integrated the WebSMS service, the proper contexts are:

```ini
[astqueue]
channel_context = dp_entry_channel_open
context         = dp_entry_trunk_texting
```

### Configuring WebSMS, websms.conf

The WebSMS configuration is kept in `websms.conf`. This file is parsed by [PHP](https://secure.php.net/manual/en/function.parse-ini-file.php), which luckily, accepts a syntax similar to Asterisk's configuration files.
One difference is that the strings, "yes", "no", "true", "false" and "null" have to be within quotation marks otherwise they will be interpreted as Boolean by the PHP parser. In the table below some key names end with []. The square brackets are not part pf the actual key name, instead they indicate that the key can hold multiple values allowing more than one SMS API interface to be configured.

| Section    | Key             | Default                      | Format  | Description                                                  |
| ---------- | --------------- | ---------------------------- | ------- | ------------------------------------------------------------ |
| [websms]   | auth_method []  | basic                        | string  | Authentication method to use.                                |
| [websms]   | auth_secret []  |                              | string  | Authentication password/secret.                              |
| [websms]   | auth_user []    |                              | string  | Authentication user/id.                                      |
| [websms]   | key_body []     | Body                         | string  | HTTP POST key name holding the SMS message.                  |
| [websms]   | key_from []     | From                         | string  | HTTP POST key name holding SMS originating phone number.     |
| [websms]   | key_secret []   |                              | string  | HTTP POST key name holding password/secret with auth_method=plain. |
| [websms]   | key_to []       | To                           | string  | HTTP POST key name holding SMS destination phone number.     |
| [websms]   | key_user []     |                              | string  | HTTP POST key name holding user/id with auth_method=plain.   |
| [websms]   | resp_check []   |                              | string  | HTTP POST key=value to check, eg "status=success".           |
| [websms]   | url_host []     | http://localhost             | URL     | Scheme and host of the ITSP SMS API, eg https://api.example.com |
| [websms]   | url_path []     | /                            | URL     | Path of the ITSP SMS API, eg /sms/send/                      |
| [websms]   | val_charset []  |                              | string  | Set to "UCS-2" to limit Unicode characters to U+FFFF.        |
| [websms]   | val_numform []  |                              | string  | Number format to use, eg "omit+" will omit the leading "+" in international numbers. |
| [websmsd]  | key_body []     | Body                         | string  | HTTP POST key name holding the SMS message.                  |
| [websmsd]  | key_echo []     |                              | string  | Some ITSP test that the client respond by expecting it echoing the value in this key, eg "zd_echo". |
| [websmsd]  | key_from []     | From                         | string  | HTTP POST key name holding SMS origination phone number.     |
| [websmsd]  | key_to []       | To                           | string  | HTTP POST key name holding SMS destination phone number.     |
| [websmsd]  | prox_addr       | 172.16.0.0/12                | CIDR    | Trust "prox_header" from these IPs, eg 10.0.0.0/8,172.16.0.0/12,192.168.0.0/16 |
| [websmsd]  | prox_header     | HTTP_X_FORWARDED_FOR         | string  | Behind a proxy this header hold the original client address. |
| [websmsd]  | remt_addr []    |                              | CIDR    | If defined, only listed addresses are allowed, eg 185.45.152.42,3.104.90.0/24,3.1.77.0/24 |
| [websmsd]  | resp_ack []     |                              | string  | Report success like this, eg, "<Response></Response>".       |
| [websmsd]  | url_path []     |                              | string  | If defined, only listed URL paths are allowed, eg /,/mywebhook/1. URIs must start with a "/". |
| [astqueue] | archive         | no                           | string  | Use "yes" to save call file to /var/spool/asterisk/outgoing_done |
| [astqueue] | channel_context | default                      | string  | Dialplan context to answer the call, ie set up the channel.  |
| [astqueue] | context         | default                      | string  | Dialplan context to handle the SMS.                          |
| [astqueue] | maxretries      | 0                            | integer | Number of retries before failing. 0 = don't retry if fails.  |
| [astqueue] | message_encode  | rfc3986                      | string  | Only single line allowed in call file so url-encoding message. |
| [astqueue] | outgoingdir     | /var/spool/asterisk/outgoing | string  | Directory where asterisk picks up call files.                |
| [astqueue] | priority        | 1                            | integer | Dialplan priority to handle the SMS.                         |
| [astqueue] | retrytime       | 300                          | integer | How many seconds to wait before retry.                       |
| [astqueue] | stagingdir      | /var/spool/asterisk/staging  | string  | Create call file here and then move to outgoing.             |
| [astqueue] | waittime        | 45                           | integer | How many seconds to wait for an answer before the call fails. |

### Default configuration

If the Asterisk configuration directory is empty, default configuration files will be copied there at container startup. The one relevant here is `websms.conf`.

```ini
[websms]
url_host        = api.example.com
url_path        = /sms/send/
auth_user       = user
auth_secret     = secret

[websmsd]

[astqueue]
channel_context = dp_entry_channel_open
context         = dp_entry_trunk_texting
```

### Multiple interface configuration

It is possible to define more than one SMS interface. This is useful when you subscribe to the service of more than one ITSP. For outgoing SMS, using `websms`, the interface is selected using a channel variable, `WEBSMS_INDEX`, you set on each PJSIP endpoint individually. For incoming SMS, using `websmsd`, the interface is selected based on the HTTP request parameters, `remt_addr` and/or `url_path`.

The section [Default configuration](#default-configuration) contains an example of a configuration for a single interface, which we can use as a reference. Now lets look at a configuration, `websms.conf`, with two interfaces defined.

 ```ini
[websms]
url_host     [api-1] = api.example1.com
url_path             = /sms/send/
auth_user    [api-1] = user1
auth_secret  [api-1] = secret1

url_host     [api-2] = api.example2.com
auth_user    [api-2] = user2
auth_secret  [api-2] = secret2

[websmsd]
remt_addr    [api-1] = 1.2.3.4/24
url_path     [api-1] = /incomming1

remt_addr    [api-2] = 5.6.7.8,5.6.7.9
url_path     [api-2] = /incomming2
key_body     [api-2] = text
 ```

As can be seen, parameters that are common between configurations does not need to be specified more than once, see for example the parameter `url_path` above. If a parameter is defined, using square brackets, but not for all interfaces, the default value will be used for the interfaces not defined.

#### Multiple outgoing interface configurations

The channel variable, `WEBSMS_INDEX`, needs to match one of the indexes used in the `[websms]` section. Lets look at an example snippet of `pjsip_wizard.conf`

```ini
[john.doe](tpl_softphone)
hint_exten = +12025550160
endpoint/set_var = WEBSMS_INDEX=api-1

[jane.doe](tpl_softphone)
hint_exten = +12025550183
endpoint/set_var = WEBSMS_INDEX=api-2
```

Here the endpoint `john.doe` will use the `api-1` configuration for outgoing SMS, whereas `jane.doe` will use `api-2`.

#### Multiple incoming interface configurations

For incoming SMS either the `remt_addr` and/or the `url_path` parameter needs to be defined, using square brackets, for each individual interface, if more than one is used. WebSMS matches these parameters for incoming requests to figure out which configuration to use.

Note that, the parameters `prox_addr` and `prox_header` can *only* have a single definition, i.e. *no* square brackets, since they are used before the incoming request has been analyzed and the interface is therefore not yet know.

It is not necessary to explicitly name the index in the `[websmsd]` section. If the index is omitted, the order of definitions will be important. To exemplify, this `[websmsd]` configuration is equivalent to the one above.

```ini
[websmsd]
remt_addr    [] = 1.2.3.4/24
url_path     [] = /incomming1
remt_addr    [] = 5.6.7.8,5.6.7.9
url_path     [] = /incomming2
key_body     [] = text
```

## Implementation

implementing a PHP client script, which sends HTTP SMS requests, and a server that listens for HTTP POST request form your ITSP.

Currently there can only be one WebSMS configuration, so it is not possible to send or receive SMS from more than one ITSP.

### websms.php sending SMS to ITSP

The function of `websms.php` in the SMS data flow is to transfer the message out of Asterisk onto the system of the ITSP. The underlying mechanism for this is a HTTP(S) request executed using [cURL](https://curl.haxx.se/). Admittedly, since Asterisk comes with integrated support for cURL using [libcurl](https://curl.haxx.se/libcurl/) it would be possible to implement the `websms` functionality without of going the route of calling a PHP script. The main motivation of `websms` is therefore "ease of use" since it can better leverage the companion function `websmsd`.

To describe the data flow we walk trough an example where a soft-phone (endpoint) user sends a SMS to a destination outside of the PBX. The endpoint sends a SIP MESSAGE request [RFC3428](https://tools.ietf.org/html/rfc3428) to Asterisk and a [channel](https://wiki.asterisk.org/wiki/display/AST/Channels) is set up and placed in the dial-plan. The channel variables include the, `EXTEN`, `MESSAGE(to)`, `MESSAGE(from)`, and `MESSAGE(body)`. The external destination is identified in the dial-plan and `websms.php` is call via [Asterisk Gateway Interface (AGI)](https://wiki.asterisk.org/wiki/pages/viewpage.action?pageId=32375589) in the dial-plan (extensions.conf):

```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${QUOTE(${MESSAGE(body)})},${WEBSMS_INDEX})
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

Using the PHP built-in web-server, the `websmsd.php` script listens to HTTP requests, representing incoming SMS,
from your ITSP. One such request is received a call file is generated, which will automatically be picked up by asterisk.

The PHP built-in web-server is started by issuing this command:

```bash
php -S 0.0.0.0:80 /path/websmsd.php
```
Now we describe the data flow of receiving a SMS from the ITSP for illustrative purposes. Assume that your ITSP receives a SMS addressed to your virtual number. Your ITSP forwards this SMS to your server by via its API which sends a HTTP request to the URL that you have registered with them and to which you have configured `websmsd.php` to listen to. The payload of such HTTP request might look like this:

```json
{"to":"+12025550160","from":"+15017122661","body":"Incoming message!"}
```

`websmsd.php` forwards the received SMS data, contained in the HTTP payload, to Asterisk, allowing it to pass it on to the endpoint, that is the soft-phone, by using the call file mechanism that we will describe next.

### Call files

[Call files](http://the-asterisk-book.com/1.6/call-file.html) are like a shell script for Asterisk. A user or application writes a call file into the directory `/var/spool/asterisk/outgoing/` where Asterisk processes it immediately. The call file contains all parameters needed by Asterisk to set up a channel able to carry a call or a message.

One practical limitation to consider in our case is that a message cannot span multiple lines in an Asterisk call file. To work
around that we encode ([RFC3986](https://tools.ietf.org/html/rfc3986), which obsolete [RFC2396](https://tools.ietf.org/html/rfc2396)) the message, including any special characters like line breaks it may contain. 

The structure of a [call file](https://wiki.asterisk.org/wiki/display/AST/Asterisk+Call+FIles) is illustrated by the example below, which includes a encoded MESSAGE(body).
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

To make sure Asterisk does not tries to read the call file before it is fully written,`websmsd.php`first writes to the file in a staging directory before it is moved to the directory where Asterisk pick it up.

Once Asterisk pick up the call file it creates a channel and start to execute it according what is specified in the dial plan. In the dial plan, defined by `extensions.conf` the function [`MESSAGE()`](https://wiki.asterisk.org/wiki/display/AST/Asterisk+17+Function_MESSAGE) is used to access the SMS data.
