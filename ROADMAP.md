# Road map

## Dial plan

- Sanitize all incoming extensions so they are all international

## Documentation

Needed. :)
Start with describing the seeding procedure(s).

Perhaps use separate files for Autoban and Websms?

## Music on hold
- script that converts sound files to wav 8000 Hz mono

## Autoban

- Write shell utility to add/delete IPs from `nft` state. Use shell utility name `autoban` and rename service to `autoband.php`.
- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.

## WebSMS

- Sanitize conf settings. Issue warning and use defaults when invalid settings are detected.

## Asterisk modules

check what modules are needed and avoid loading others. This will help get rid of error messages during startup.
