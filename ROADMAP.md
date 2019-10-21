# Road map

## Alpine Linux
Currently using alpine:3.9 since for alpine:3.10 there are dependency errors reported when asterisk starts, will be fixed in alpine:3.10.3 so will test then it is available.

## Documentation
Needed. :)
Start with describing the seeding procedure(s).

## Music on hold
script that converts sound files to wav 8000 Hz mono

##Autoban
- Introduce a `SIGTERM` trap in `entrypoint.sh` so that we can save the `nft` state at container shutdown.
- Write shell utility to add/delete IPs from `nft` state
