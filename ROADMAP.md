# Road map

## Signaling Security

- SIP message traffic via TLS
- SDES-SRTP
[Secure Calling Tutorial](https://wiki.asterisk.org/wiki/display/AST/Secure+Calling+Tutorial)

## Dial plan

## Documentation

Needed. :)
Start with describing the seeding procedure(s).

Perhaps use separate files for Autoban?

## Music on hold

- script that converts sound files to wav 8000 Hz mono

## Autoban

- Write to autoban.nft every time we get a security event and updates NFT, so that its state is always preserved.
- Write shell utility to add/delete IPs from `nft` state. Use shell utility name `autoban` and rename service to `autoband.php`.
- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.

If autoban.conf does not have an `[nftables]` section we get the following warning
autoban[43]: WARNING[8]: autoban.class.inc:55: Undefined index: nftables.
autoban[43]: WARNING[2]: autoban.class.inc:55: array_merge(): Expected parameter 2 to be an array, null given.


## WebSMS

- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.

## Asterisk modules

check what modules are needed and avoid loading others. This will help get rid of error messages during startup.
