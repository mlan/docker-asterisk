# Road map

## Documentation

- privatedial.md
- README.md

## PrivateDial

- Check that callid is set correctly on incoming calls (from trunk).
- Test MiniVM functionality.
- Debug MiniVM access messages.
- Make IVR custom-able

### Put in documentation

- Note, Presence is only supported for Sangama/Digium phones, not for softphones.

## Music on hold

- Script that converts sound files to wav 8000 Hz mono.

## AutoBan

- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.
- Perhaps replace entrypoint.d/ with /etc/conf.d/nftables?

## WebSMS

- Allow multi-configs.
- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.
- Handle "Invalid request" errors.

### WebSMS Multi-Configs

Parameters can be arrays.
websms.conf
```ini
[websms]
host[trunk1]    = https://api.trunk1.com
host[trunk2]    = https://api.trunk2.com
path[trunk1]    = /sms/send
path[trunk2]    = 
```
```bash
same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${QUOTE(${MESSAGE(body)})},${ENDPOINT_TRUNK})
```

### WebSMSd Multi-Configs

- match incoming ip and or path to split off requests into using different configutation

array key can be omitted, if so indexes will automatically be assiged, so order is important.
websms.conf
```ini
[websmsd]
remote_addr[]   = 1.2.3.4/24
request_uri[]   = /trunk1
remote_addr[]   = 5.6.7.8
request_uri[]   = /trunk2
```

## Asterisk modules

- Check what modules are needed and avoid loading others. This will help get rid of error messages during startup.

## Dependencies

- Reorganize. Use git submodules?
