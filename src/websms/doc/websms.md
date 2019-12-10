#  WebSMS

The [SIP](wikipedia.org/wiki/Session_Initiation_Protocol) includes the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol providing [IM](wikipedia.org/wiki/Instant_messaging) or [SMS](wikipedia.org/wiki/SMS), which Asterisk supports natively. Unfortunately many [ITSP](wikipedia.org/wiki/Internet_telephony_service_provider) does not offer SIMPLE but instead sends and receives SMS using HTTP requests. The `websmsd` service bridges this imitation, by implementing a PHP a client script, which sends HTTP SMS requests, and a server that listens for HTTP POST request form your ISTP provider.

## Operation

The `websmsd` client forwards the SMS to the appropriate soft phone using `call files`. Similarly, the `websms.php` script is called from the Asterisk dial plan to connect you your ITSP API to send SMS.

## Configuration


| File name        | Description                                                  |
| ---------------- | ------------------------------------------------------------ |
| websms.conf      | Define HTTP SMS, incoming and outgoing                       |

Most ITSP requires outgoing authentication and some accept outgoing authentication.

Both the SMS receiving service and the SMS sending scripts are configured in the file `websms.conf`

### Configuring WebSMS, websms.conf

WebSMS configuration is kept in `websms.conf`. This file hold user customization of the websms originate and termination.
This is a php ini file. Luckily its syntax is similar to other asterisk conf files.
"yes" and "no" have to be within quotation marks otherwise they will be interpreted as Boolean.

| Section    | Key             | Default                      | Format  | Description                                                  |
| ---------- | --------------- | ---------------------------- | ------- | ------------------------------------------------------------ |
| [websms]   | host            |                              | URI     | protocol and host name to send sms to                        |
| [websms]   | path            |                              | string  | complete url will be <host><path>                            |
| [websms]   | key_to          | To                           | string  | http POST key name holding sms destination phone number      |
| [websms]   | key_from        | From                         | string  | http POST key name holding sms originating phone number      |
| [websms]   | key_body        | Body                         | string  | http POST key name holding the sms message                   |
| [websms]   | auth_user       |                              | string  | authentication username/key                                  |
| [websms]   | auth_passwd     |                              | string  | authentication password/secret                               |
| [websms]   | auth_method     | basic                        | string  | eg "zadarma" method to authenticate sms request              |
| [websms]   | response_check  |                              | string  | http POST key=value to check, eg "status=success"            |
| [websms]   | number_format   |                              | string  | eg "omit+" omit leading "+" in phone numbers                 |
| [websmsd]  | key_to          | To                           | string  | http POST key name holding sms destination phone number      |
| [websmsd]  | key_from        | From                         | string  | http POST key name holding sms origination phone number      |
| [websmsd]  | key_body        | Body                         | string  | http POST key name holding the sms message                   |
| [websmsd]  | key_echo        |                              | string  | some ITSP test that the client respond by echoing it value, eg "zd_echo" |
| [websmsd]  | key_account     |                              | string  | NOT USED                                                     |
| [websmsd]  | report_success  |                              | string  | report success, eg, "<Response></Response>"                  |
| [websmsd]  | permit_addr     |                              | string  | if defined, only listed addrs are accepted, eg 185.45.152.42,3.104.90.0/24,3.1.77.0/24 |
| [websmsd]  | proxy_addr      | 172.16.0.0/12                | CIDR    | Trust "proxy_header" from these IPs, eg 10.0.0.0/8,172.16.0.0/12,192.168.0.0/16 |
| [websmsd]  | proxy_header    | HTTP_X_FORWARDED_FOR         | string  | Behind a proxy this header hold the real client IP           |
| [astqueue] | outgoingdir     | /var/spool/asterisk/outgoing | string  | directory where asterisk picks up call files                 |
| [astqueue] | stagingdir      | /var/spool/asterisk/staging  | string  | create call file here and then move to outgoing              |
| [astqueue] | waittime        | 45                           | integer | how many seconds to wait for an answer before the call fails |
| [astqueue] | maxretries      | 0                            | integer | number of retries before failing. 0 = don't retry if fails   |
| [astqueue] | retrytime       | 300                          | integer |                                                              |
| [astqueue] | archive         | no                           | string  | "yes" = save call file to /var/spool/asterisk/outgoing_done  |
| [astqueue] | channel_context | default                      | string  | dialplan context to answer the call, ie set up the channel   |
| [astqueue] | context         | default                      | string  | dialplan context to handle the sms                           |
| [astqueue] | priority        | 1                            | integer | dialplan priority to handle the sms                          |

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

### websms
The websms PHP script takes command line arguments and generates a (curl) HTTP
request to the ITSP web API which will send SMS.

Call via AGI in extensions.conf:
```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${MESSAGE(body)})
```
### websmsd
This PHP script listens to http requests, representing incoming SMS,
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
