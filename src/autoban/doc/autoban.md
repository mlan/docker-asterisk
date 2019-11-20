#  AutoBan, automatic firewall

The Autoban service listens to Asterisk security events on the AMI interface. Autoban is activated if there is an `autoban.conf` file and that the parameter `enabled` within is not set to `no`. When one of the `InvalidAccountID`, `InvalidPassword`, `ChallengeResponseFailed`, or `FailedACL` events occur Autoban start to watch the source IP address for `watchtime` seconds. If more than `maxcount` security events occurs within this time, all packages from the source IP address is dropped for `jailtime` seconds. When the `jailtime` expires packages are gain accepted from the source IP address, but for additional `watchtime` seconds this address is on "parole". Is a security event be detected from this address during the "parole" period it is immediately blocked again, for a progressively longer time. This progression is configured by `repeatmult`, which determines how many times longer the IP is blocked. To illustrate, first assume `jailtime=20m` and `repeatmult=6`, then the IP is blocked 20min the first time, 2h (120min) the second, 12h (720min) the third, 3days (4320min) the forth and so on. If no security event is detected during the "parole" the IP is no longer being watched.


## Configuration
#### `autoban.conf`

```ini
[asmanager]
server     = 127.0.0.1
port       = 5038
username   = autoban
secret     = 6003.438

[autoban]
enabled    = true
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

# Implementation
## autoban
The autoban PHP script takes command line arguments

### Usage

```bash
autoban -h
```
## autoband

The Autoban service listens to Asterisk security events on the AMI interface

### Usage

Run with the PHP built-in web server:
```bash
 /path/autoban.php
```
