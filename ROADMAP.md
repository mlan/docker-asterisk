# Road map

## Alpine Linux
Currently using alpine:3.9 since for alpine:3.10 there are dependency errors reported when asterisk starts, will be fixed in alpine:3.10.3 so will test then it is available.

## Documentation
Needed. :)
Start with describing the seeding procedure(s).

## Music on hold
script that converts sound files to wav 8000 Hz mono

## Heath check
arrange

## SMS

- Refactor code to use sms.class.inc and perhaps astqueue.class.inc.
- Add IP filter in smsd.php
- Add signature test in smsd.php

##Autoban

- Read and write time strings which has the the format: $v_1 \mathbf{d} v_2 \mathbf{h} v_3 \mathbf{m} v_4 \mathbf{s}$
- Use `timeout` parameter in `parole` to figure out how many times we have jailed an IP. If we do this we can get rid off the counters in php, so that code becomes stateless.
- Introduce a `SIGTERM` trap in `entrypoint.sh` so that we can save the `nft` state at container shutdown.
- Write shell utility to add/delete IPs from `nft` state
