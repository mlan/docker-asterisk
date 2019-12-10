# 0.5.0

- [acme](src/acme) Fixed dumpcert.sh leaking to stdout. Have it write to logger instead.
- [autoban](src/autoban) Added utility autoban, which helps to manage the NFT state
- [autoban](src/autoban) Code base now refactored and split into autoban.class.inc and nft.class.inc

# 0.4.0

- [privatedial](src/privatedial) Now keep main dial-plan conf files separate.
- [privatedial](src/privatedial) Start to document the PrivateDial dial-plan.
- [autoban](src/autoban) Now don't crash if autoban.conf does not have both an `[autoban]` and an `[nftables]` section.
- [autoban](src/autoban) Renamed autoband.php (it was autoban.php)
- [autoban](src/autoban) Updated documentation.
- [asterisk](src/asterisk) Added Networking section in README.md.

# 0.3.0

- [acme](src/acme) New support for [Let’s Encrypt](https://letsencrypt.org/) LTS certificates using [Traefik](https://traefik.io/) using `ACME_FILE=/acme/acme.json`.
- [asterisk](src/asterisk) Configuration now supports UDP, TCP and TLS and SDES.
- [asterisk](src/asterisk) Generate self-signed TLS certificate.
- [asterisk](src/asterisk) Improved structure of `pjsip_wizard.conf`.
- [asterisk](src/asterisk) Don't answer when device is UNAVAILABLE in `[dp_channel_answer]`
- [docker](src/docker) The [setup-runit.sh](src/docker/bin/setup-runit.sh) script now have options:  down, force, log, name, source, quiet.
- [websms](src/websms) Added `number_format` parameter. Set to `omit+` to strip phone numbers from leading +.

# 0.2.1

- [asterisk](src/asterisk) Sanitize incoming extensions so they are all international
- [asterisk](src/asterisk) Move APP_SMS global to extensions.conf
- [websms](src/websms) Use `$_POST` since `file_get_contents("php://input")` cannot handle multipart/form-data
- [websms](src/websms) Allow IP addr filtering behind proxy by using HTTP_X_FORWARDED_FOR
- [websms](src/websms) websmsd.php parameters are json decoded and searched recursively
- [websms](src/websms) Also support Zadarma POST parameters in websms.class.inc
- [websms](src/websms) Started WebSMS (separate) documentation
- [autoban](src/autoban) Fixed new bug in autoban.class.inc
- [autoban](src/autoban) Added conf sample file autoban.conf.sample

# 0.2.0

- [repo](src) Now reorganize repo files according to which service they provide
- [docker](Dockerfile) alpine 3.10.3 released so now build using alpine:3.10
- [docker](Dockerfile) Added Health check
- [docker](src/docker) Introduce a `SIGTERM` trap in `entrypoint.sh` allowing graceful container termination with `exitpoint.d` script execution
- [docker](src/docker) [setup-runit.sh](src/docker/bin/setup-runit.sh) now also take switches -n and -l.
- [docker](src/docker) We now create directory structure when an empty volume is mounted at /srv.
- [asterisk](src/asterisk) Based on live testing updated templates in pjsip_wizard.conf
- [asterisk](src/asterisk) Now use extensions-local.conf to keep all local configurations
- [asterisk](src/asterisk) Fixed typo in rtp.conf
- [websms](src/websms) Retired service sms/d which has been succeeded by websms/d
- [websms](src/websms) New verify POST request in websms.class.inc
- [websms](src/websms) New check source IP address in websms.class.inc
- [autoban](src/autoban) New service Autoban, which listens to security AMI events and dynamically configures nftables to block abusing IPs.
- [autoban](src/autoban) autoban.class.inc (formerly nft.class.inc) is now state less
- [autoban](src/autoban) Restricting Autoban's AMI access to a minimum
- [autoban](src/autoban) Autoban now has `repeatmult` punishing repeat offenders progressively more severely
- [autoban](src/autoban) Autoban now use nftables timeouts
- [autoban](src/autoban) Added `entrypoint.d`  and  `exitpoint.d` scripts so that the `nft` state is loaded/saved at container startup/shutdown.

# 0.1.0

- Using alpine:3.9 since for alpine:3.10 there are dependency errors reported when asterisk starts
- minivm-send bash script simplify minivm configuration
