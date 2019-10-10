#!/bin/bash
#
# This script creates an acl-blacklist.conf

DOCKER_VOIPBL_FILE=${DOCKER_VOIPBL_FILE-/srv/var/spool/asterisk/acl/acl-blacklist.conf}

wget -qO - http://www.voipbl.org/update/ |
sed 's/^/deny=/' > $DOCKER_VOIPBL_FILE
