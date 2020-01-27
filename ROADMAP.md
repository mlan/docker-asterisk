# Road map

## Documentation

Needed. :)

- websms.md
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

- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.
- Handle "Invalid request" errors.

## Asterisk modules

- Check what modules are needed and avoid loading others. This will help get rid of error messages during startup.

## Dependencies

- Reorganize. Use git submodules?

## Docker

- Harmonize script stdout messages. Separate script /usr/local/bin/inform ?
