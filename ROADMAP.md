# Road map

## PrivateDial

- Make IVR custom-able

## MiniVM

The application [MinivmGreet()](https://wiki.asterisk.org/wiki/display/AST/Asterisk+16+Application_MinivmGreet) is broken. Now it only plays the "temp" message.
Consider fixing the code in [app_minivm.c](https://github.com/asterisk/asterisk/blob/8f5534a68a01ad3fbe6b1920c8ab160fc3b4df89/apps/app_minivm.c) lines 2326-2345 and file a patch.

## Upgrade utility

- Shell utility that helps upgrading config files.

## Music on hold

- Script that converts sound files to wav 8000 Hz mono.

## AutoBan

- Now `.nft` file is loaded at container startup (entry.d) irrespective of whether autoband is enabled or not. Only load `.nft` files if autoband is enabled.
- Add option to get reverse DNS using gethostbyaddr($ip); in `show who`.
- Perhaps replace entry.d/ with /etc/conf.d/nftables?
- Allow intervals, eg 192.168.1.1-192.168.1.200, in blacklist and whitelist.

## WebSMS

## Asterisk modules
