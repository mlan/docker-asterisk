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
- [docker](Dockerfile) alpine 3.10.3 released so now build using alpine:3.10
- [docker](Dockerfile) Added Health check
- [docker](Dockerfile) Introduce a `SIGTERM` trap in `entrypoint.sh` allowing graceful container termination with `exitpoint.d` script execution
- [docker](Dockerfile) Now reorganize repo files according to which service they provide
- [docker](Dockerfile) src/bin/setup-runit.sh now also take switches -n and -l.
- [docker](Dockerfile) We now create directory structure when empty volume is mounted at /srv.
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
