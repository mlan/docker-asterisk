#!/usr/bin/env sh
# Copyright (c) 2017 Brian 'redbeard' Harrington <redbeard@dead-city.org>
#
# dumpcerts.sh - A simple utility to explode a Traefik acme.json file into a
#                directory of certificates and a private key
#
# Usage - dumpcerts.sh /etc/traefik/acme.json /etc/ssl/
#
# Dependencies -
#   util-linux
#   openssl
#   jq
# The MIT License (MIT)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#

# Exit codes:
# 1 - A component is missing or could not be read
# 2 - There was a problem reading acme.json
# 4 - The destination certificate directory does not exist
# 8 - Missing private key

#set -o errexit
##set -o pipefail
#set -o nounset
#set -o verbose


#
# configuration
#
source docker-common.sh

DOCKER_ACME_SSL_DIR=${DOCKER_ACME_SSL_DIR-/etc/ssl/acme}
ACME_FILE=${ACME_FILE-/acme/acme.json}

CMD_DECODE_BASE64="base64 -d"

#
# functions
#
usage() { echo "$(basename $0) <path to acme> <destination cert directory>" ;}

test_args() {
	# when called by inotifyd the first argument is the single character 
	# event desciptior, lets drop it
	dc_log 7 "Called with args $@"
	[ $# -ge 0 ] && [ ${#1} -eq 1 ] && shift
	readonly acmefile="${1-$ACME_FILE}"
	readonly certdir="${2-$DOCKER_ACME_SSL_DIR}"
}

test_dependencies() {
	# Allow us to exit on a missing jq binary
	if dc_is_installed jq; then
		dc_log 7 "The package jq is installed."
	else
		dc_log 4 "You must have the binary jq to use this."
		exit 1
	fi
}

test_acmefile() {
	if [ ! -r "${acmefile}" ]; then
		dc_log 4 "There was a problem reading from (${acmefile}). We need to read this file to explode the JSON bundle... exiting."
		exit 2
	fi
}

test_certdir() {
	if [ ! -d "${certdir}" ]; then
		dc_log 4 "Path ${certdir} does not seem to be a directory. We need a directory in which to explode the JSON bundle... exiting."
		exit 4
	fi
}

make_certdirs() {
	# If they do not exist, create the needed subdirectories for our assets
	# and place each in a variable for later use, normalizing the path
	mkdir -p "${certdir}/certs" "${certdir}/private"
	pdir="${certdir}/private"
	cdir="${certdir}/certs"
}

bad_acme() {
	dc_log 4 "There was a problem parsing your acme.json file."
	exit 2
}

read_letsencryptkey() {
	# look for key assuming acme v2 format
	priv=$(jq -e -r '.[].Account.PrivateKey' "${acmefile}" 2>/dev/null)
	if [ $? -eq 0 ]; then
		acmeversion=2
		dc_log 7 "Using acme v2 format, the PrivateKey was found in ${acmefile}"
	else
		# look for key assuming acme v1 format
		priv=$(jq -e -r '.Account.PrivateKey' "${acmefile}" 2>/dev/null)
		if [ $? -eq 0 ]; then
			acmeversion=1
			dc_log 7 "Using acme v1 format, the PrivateKey was found in ${acmefile}"
		else
			dc_log 4 "There didn't seem to be a private key in ${acmefile}. Please ensure that there is a key in this file and try again."
			exit 2
		fi
	fi
}

save_letsencryptkey() {
	local keyfile=${pdir}/letsencrypt.key
	printf -- \
		"-----BEGIN RSA PRIVATE KEY-----\n%s\n-----END RSA PRIVATE KEY-----\n" \
		${priv} | fold -w 65 | \
		openssl rsa -inform pem -out $keyfile 2>/dev/null
	if [ -e $keyfile ]; then
		dc_log 7 "PrivateKey is valid and saved in $keyfile"
	else
		dc_log 4 "PrivateKey appers NOT to be valid"
		exit 2
	fi
}

read_domains() {
	# Process the certificates for each of the domains in acme.json
	case $acmeversion in
		1) jq_filter='.Certificates[].Domain.Main' ;;
		2) jq_filter='.[].Certificates[].domain.main' ;;
	esac
	domains=$(jq -r $jq_filter $acmefile)
	if [ -n "$domains" ]; then
		dc_log 7 "Extracting private key and cert bundle for domains $domains."
	else
		dc_log 4 "Unable to find any domains in $acmefile."
		exit 2
	fi
}

save_certs() {
	# Traefik stores a cert bundle for each domain.  Within this cert
	# bundle there is both proper the certificate and the Let's Encrypt CA
	dc_log 5 "Extracting private keys and cert bundles in ${acmefile}"
	case $acmeversion in
		1)
			jq_crtfilter='.Certificates[] | select (.Domain.Main == $domain )| .Certificate'
			jq_keyfilter='.Certificates[] | select (.Domain.Main == $domain )| .Key'
			;;
		2)
			jq_crtfilter='.[].Certificates[] | select (.domain.main == $domain )| .certificate'
			jq_keyfilter='.[].Certificates[] | select (.domain.main == $domain )| .key'
			;;
	esac
	for domain in $domains; do
		crt=$(jq -e -r --arg domain "$domain" "$jq_crtfilter" $acmefile) || bad_acme
		echo "${crt}" | ${CMD_DECODE_BASE64} > "${cdir}/${domain}.crt"
		key=$(jq -e -r --arg domain "$domain" "$jq_keyfilter" $acmefile) || bad_acme
		echo "${key}" | ${CMD_DECODE_BASE64} > "${pdir}/${domain}.key"
	done
}


#
# run
#
test_args $@
test_dependencies
test_acmefile
test_certdir
read_letsencryptkey
make_certdirs
save_letsencryptkey
read_domains
save_certs
