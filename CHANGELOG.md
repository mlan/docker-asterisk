# 1.1.5

- [demo](demo/Makefile) Now depend on the docker-compose-plugin.
- [build](Makefile) Set `DOCKER_BUILDKIT=0` to make `docker build` handle our symbolic links as we intended.


# 1.1.4

- [privatedial](src/privatedial) Allow all TLS protocols in the `minivm-send` bash script.
- [test](test/Makefile) Now also test the PHP is setup correctly.

# 1.1.3

- [repo](src) Fix the bug; websms and autoban not working. We need to use softlinks to ../../share/php81/.
- [docker](README.md) Corrected misspelling.

# 1.1.2

- [build](Dockerfile) Fix the bug; websmsd not working. We need to set DOCKER_PHP_DIR=/usr/share/php81.

# 1.1.1

- [build](Makefile) Now use alpine:3.17 (asterisk:18.15.0).
- [build](Dockerfile) Switch to php81.
- [test](.travis.yml) Updated dist to jammy.

# 1.1.0

- [build](Makefile) Now use alpine:3.16 (asterisk:18.11.2).
- [build](Dockerfile) Switch to php8.
- [demo](demo) Switch to php8.
- [privatedial](src/privatedial) BREAKING. In `extensions.conf` now use `APP_SMS = /usr/local/bin/websms`.

# 1.0.0

- [docker](src/docker) Now use alpine:3.15 (asterisk:18.2.2).
- [autoban](src/autoban) Let autoban manipulate nft without breaking docker networking. Since docker 5:20.10 container DNS resolve is based on nft rules (with iptables-nft) which autoban's nft implementation interfered with resulting in container unable to resolve network names.
- [autoban](src/autoban) Now use DOCKER_NFT_DIR=/etc/nftables.d.
- [autoban](src/autoban) Only load `.nft` files if nft is installed.
- [build](Makefile) Now push images to registry.
- [demo](demo) Updated demo to also work with docker-compose >= 2.
- [test](test/Makefile) Added container name-space network inspection targets.

# 0.9.9

- [docker](src/docker) Now use alpine:3.14 (asterisk:18.2.2).
- [docker](ROADMAP.md) Use [travis-ci.com](https://travis-ci.com/).

# 0.9.8

- [autoban](src/autoban) Let autoband handle AMI connection failures nicely.

# 0.9.7

- [docker](src/docker) Now use alpine:3.13 (asterisk:18.1.1).
- [test](test/Makefile) Move tests into test dir.

# 0.9.6

- [repo](hooks) Fixed bug in hooks/pre_build. Use curl in `make pre_build`.

# 0.9.5

- [codec](sub/codec) Provide the [G.729](https://en.wikipedia.org/wiki/G.729) and [G.723.1](https://en.wikipedia.org/wiki/G.723.1) audio codecs.
- [codec](sub/codec) Improved handling of codec versions (`BLD_CVER` in Makefile).

# 0.9.4

- [websms](src/websms) Use `prox_addr = 172.16.0.0/12,192.168.0.0/16` by default.

# 0.9.3

- [acme](src/acme) Introduce `ACME_POSTHOOK="sv restart asterisk"` and run that after we have updated the certificates.
- [docker](src/docker) Don't move `DOCKER_APPL_SSL_DIR=$DOCKER_SSL_DIR/asterisk` to persistent storage. Data there is updated at container startup anyway. Moreover there is no need to remove old data when it is updated.
- [privatedial](src/privatedial) In `pjsip_transport.conf` set `method=tlsv1_2` to harden TLS.

# 0.9.2

- [docker](src/docker) Use the native envvar `SVDIR` instead of `DOCKER_RUNSV_DIR`.
- [docker](src/docker) Update docker-common.sh.
- [docker](src/docker) Now use docker-config.sh.
- [docker](src/docker) Update docker-entrypoint.sh.
- [docker](src/docker) Update docker-service.sh.
- [docker](src/docker) Now use DOCKER_ENTRY_DIR=/etc/docker/entry.d and DOCKER_EXIT_DIR=/etc/docker/exit.d.
- [docker](Makefile) Improved smoke test.
- [acme](src/acme/bin/acme-extract.sh) Update module.
- [privatedial](src/privatedial) Breaking change. Now use `cert_file=/etc/ssl/asterisk/cert.pem` and `priv_key_file=/etc/ssl/asterisk/priv_key.pem`

# 0.9.1

- [repo](hooks) Added hooks/pre_build which assembles files from sub-modules.
- [repo](.travis.yml) Revisited `.travis.yml`.
- [docker](README.md) Proofread documentation.
- [docker](README.md) Fixed broken hyperlinks in documentation.

# 0.9.0

- [privatedial](src/privatedial) Break out endpoints from pjsip_wizard.conf to pjsip_endpoint.conf.
- [privatedial](src/privatedial) Use Hangup() instead of Goto() when entering extension `h`.
- [privatedial](src/privatedial) Work around bug in [MinivmGreet()](https://wiki.asterisk.org/wiki/display/AST/Asterisk+16+Application_MinivmGreet).
- [privatedial](src/privatedial) Renamed dialplan contexts.
- [privatedial](src/privatedial) Dialplan `[sub_voicemail]` now handles CHANUNAVAIL correctly.
- [privatedial](src/privatedial) Added `endpoint/bind_rtp_to_media_address = yes`
- [docker](README.md) Complete documentation.
- [docker](src/docker) Now use alpine:3.12 (asterisk:16.7.0).
- [websms](src/websms) `WEBSMSD_PORT=80` sets PHP web server port, used by WebSMS.
- [repo](src) Harmonized file names in `entry.d` and `exit.d`.
- [repo](sub) Use git submodule for third party projects.

# 0.8.0

- [websms](src/websms) Harmonized configuration parameter names.
- [websms](src/websms) Harmonized function names.
- [websms](src/websms) Facilitate static key-value pairs, `val_static = "key1=value1,key2=value2"`.
- [websms](src/websms) Parameter `val_numform`, now takes `E.164` (omit +) and `E.123`.
- [websms](src/websms) Improved Unicode configuration, allowing `val_unicode = "key=value"`.
- [websms](src/websms) Added authorization methods, `plain` and `none`.
- [websms](src/websms) Allow multiple API interfaces to be configured.
- [websms](src/websms) Now accept incoming message with null body.
- [websms](src/websms) Code clean up.
- [privatedial](src/privatedial) Use set_var=TRUNK_ENDPOINT to set outgoing target for each endpoint individually.
- [privatedial](src/privatedial) Don't use `endpoint/from_user`, it overwrites CallerID.

# 0.7.0

- [acme](src/acme/bin/acme-extract.sh) Support both v1 and v2 formats of the acme.json file.
- [acme](src/acme/entry.d/50-acme-monitor-tlscert) Support both host and domain wildcard TLS certificates.
- [websms](src/websms) Complete documentation.
- [privatedial](src/privatedial) Advancing documentation.
- [docker](README.md) Advancing documentation.
- [docker](src/notused) Cleanup `src/notused`.
- [docker](src/docker) Consolidate common functions in src/docker/bin/docker-common.sh.

# 0.6.0

- [docker](Dockerfile) Audio via PulseAudio.
- [docker](src/docker) Now use alpine:3.11 (asterisk:16.6.2).
- [demo](demo) Added demo.
- [demo](demo) Enabled audio via PulseAudio socket and cookie.
- [demo](demo) Use host timezone by mounting /etc/localtime.
- [websms](src/websms) Updating documentation.
- [privatedial](src/privatedial) Added demo-echotest in IVR.
- [privatedial](src/privatedial) Fixed initiation issue for minivm.

# 0.5.2

- [websms](src/websms) Fixing bugs related to special characters in SMS messages
- [websms](src/websms) Added `val_unicode` parameter. Set to `ucs-2` to make sure all characters are within the Unicode BMP (up to U+FFFF).
- [websms](src/websms) Updating documentation.
- [websms](src/websms) Refactoring of `astqueue.class.ini` to better cope with message encoding.
- [privatedial](src/privatedial) added `sub_decode_body` to cope with encoded messages.

# 0.5.1

- [docker](Makefile) Enable PHP profiling using xdebug.
- [autoban](src/autoban) Optimized code with respect to efficiency and speed.
- [autoban](src/autoban) Improved command line options of the shell utility.

# 0.5.0

- [acme](src/acme) Fixed dumpcert.sh leaking to stdout. Have it write to logger instead.
- [autoban](src/autoban) Added shell utility autoban, which helps to manage the NFT state
- [autoban](src/autoban) Updated documentation.
- [autoban](src/autoban) Now write to autoban.nft every time we get a security event and update NFT, so that its state is always preserved.
- [autoban](src/autoban) Code base now refactored and split into autoban.class.inc and nft.class.inc
- [websms](src/websms) Updated documentation.

# 0.4.0

- [privatedial](src/privatedial) Now keep main dial-plan conf files separate.
- [privatedial](src/privatedial) Start to document the PrivateDial dial-plan.
- [autoban](src/autoban) Now don't crash if autoban.conf does not have both an `[autoban]` and an `[nftables]` section.
- [autoban](src/autoban) Renamed autoband.php (it was autoban.php)
- [autoban](src/autoban) Updated documentation.
- [asterisk](src/asterisk) Added Networking section in README.md.

# 0.3.0

- [acme](src/acme) New support for [Let’s Encrypt](https://letsencrypt.org/) TLS certificates using [Traefik](https://traefik.io/) using `ACME_FILE=/acme/acme.json`.
- [asterisk](src/asterisk) Configuration now supports UDP, TCP and TLS and SDES.
- [asterisk](src/asterisk) Generate self-signed TLS certificate.
- [asterisk](src/asterisk) Improved structure of `pjsip_wizard.conf`.
- [asterisk](src/asterisk) Don't answer when device is UNAVAILABLE in `[dp_answer]`
- [docker](src/docker) The [docker-service.sh](src/docker/bin/docker-service.sh) script now have options:  down, force, log, name, source, quiet.
- [websms](src/websms) Added `val_numform` parameter. Set to `E164` to strip phone numbers from leading +.

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
- [docker](src/docker) Introduce a `SIGTERM` trap in `docker-entrypoint.sh` allowing graceful container termination with `exit.d` script execution
- [docker](src/docker) [docker-service.sh](src/docker/bin/docker-service.sh) now also take switches -n and -l.
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
- [autoban](src/autoban) Added `entry.d`  and  `exit.d` scripts so that the `nft` state is loaded/saved at container startup/shutdown.

# 0.1.0

- [docker](Dockerfile) Using alpine:3.9 since for alpine:3.10 there are dependency errors reported when asterisk starts.
- [privatedial](src/privatedial) minivm-send bash script simplify minivm configuration.
