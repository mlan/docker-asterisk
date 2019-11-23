#  WebSMS

The [SIP](wikipedia.org/wiki/Session_Initiation_Protocol) includes the [SIMPLE](wikipedia.org/wiki/SIMPLE_(instant_messaging_protocol)) protocol providing [IM](wikipedia.org/wiki/Instant_messaging) or [SMS](wikipedia.org/wiki/SMS), which Asterisk supports natively. Unfortunately many [ITSP](wikipedia.org/wiki/Internet_telephony_service_provider) does not offer SIMPLE but instead sends and receives SMS using HTTP requests. The `websmsd` service bridges this imitation, by implementing a PHP a client script, which sends HTTP SMS requests, and a server that listens for HTTP POST request form your ISTP provider. The `websmsd` client forwards the SMS to the appropriate soft phone using `call files`. Similarly, the `websms.php` script is called from the Asterisk dial plan to connect you your ITSP API to send SMS. 

## Configuration

### Configuration files overview

| File name        | Description                                                  |
| ---------------- | ------------------------------------------------------------ |
| websms.conf      | Define HTTP SMS, incoming and outgoing                       |

Most ITSP requires outgoing authentication and some accept outgoing  authentication.  

Both the SMS receiving service and the SMS sending scripts are configured in the file `websms.conf`

```ini
; websms.conf.sample
;
; This file hold user customization of the websms originate and termination.
; This is a php ini file. Luckily its syntax is similar to other asterisk conf files.
; "yes" and "no" have to be within quotation marks otherwise they will be 
; interpreted as Boolean.
;

[websms]
host            = "https://api.example.com" ;protocol and host name to send sms to
path            = "/sms/send/" ;complete url will be <host><path>
key_to          = "To"   ;http POST key name holding sms destination phone number
key_from        = "From" ;http POST key name holding sms orignating phone number
key_body        = "Body" ;http POST key name holding the sms message
auth_user       = "usename" ;autentcion username/key
auth_passwd     = "passwd"  ;authentication password/secret
auth_method     = "basic"   ;eg "zadarma" method to authenticate sms request
response_check  = ""        ;http POST key=value to check, eg "status=success"

[websmsd]
key_to          = "To"   ;http POST key name holding sms destination phone number
key_from        = "From" ;http POST key name holding sms orignating phone number
key_body        = "Body" ;http POST key name holding the sms message
key_echo        = "" ;some ITSP test that the client respond by echoing it value, eg "zd_echo"
key_account     = "" ;
report_success  = "" ;report success, eg, "<Response></Response>"
permit_addr     = "" ;if defined, only listed addrs are accepted, eg 185.45.152.42,3.104.90.0/24,3.1.77.0/24
proxy_addr      = "172.16.0.0/12" ; Trust "proxy_header" from these IPs, eg 10.0.0.0/8,172.16.0.0/12,192.168.0.0/16
proxy_header    = "HTTP_X_FORWARDED_FOR" ; Behind a proxy this header hold the real client IP

[astqueue]
outgoingdir     = /var/spool/asterisk/outgoing ;directory where asterisk picks up call files
stagingdir      = /var/spool/asterisk/staging  ;create call file here and then move to outgoing
waittime        = 45      ;how many seconds to wait for an answer before the call fails
maxretries      = 0       ;number of retries before failing. 0 = don't retry if fails
retrytime       = 300     ;how many seconds to wait before retry
archive         = "no"    ;"yes" = save call file to /var/spool/asterisk/outgoing_done
channel_context = default ;dialplan context to answer the call, ie set up the channel
context         = default ;dialplan context to handle the sms
priority        = 1       ;dialplan priority to handle the sms
```
# Implementation
## websms
The websms PHP script takes command line arguments and generates a (curl) HTTP
request to the ITSP web API which will send SMS.

### Usage
Call via AGI in extensions.conf:
```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${MESSAGE(body)})
```
## websmsd
 This PHP script listens to http requests, representing incoming SMS,
 from your ITSP and generate call files which will be picked up by asterisk.

### Usage
Run with the PHP built-in web server:
```bash
 php -S 0.0.0.0:80 /path/websmsd.php
```
 Outline
 Define error handler and load variable values.
 Respond to echo requests.
 Read the post header data
 Generate call file name.
 Create new call file in the staging directory.
 Move the call file to the outgoing directory, so that Asterisk pick it up.
 Respond with a status message.
