#!/bin/bash
# /usr/local/bin/submittmail.sh
#

from=${1-voicemail-noreply@example.com}
mxurl=${2-mx.example.com:587}
user=${3-mxuser}
pass=${4-mxpasswd}

sendmail -t -f $from \
-H 'openssl s_client -quiet -tls1 -starttls smtp -connect '$mxurl \
-au$user -ap$pass || exit 0
