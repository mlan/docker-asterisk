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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Exit codes:
# 1 - A component is missing or could not be read
# 2 - There was a problem reading acme.json
# 4 - The destination certificate directory does not exist
# 8 - Missing private key

#set -o errexit
##set -o pipefail
set -o nounset
#set -o verbose

DOCKER_ACME_SSL_DIR=${DOCKER_ACME_SSL_DIR-/etc/ssl/acme}
ACME_FILE=${ACME_FILE-/acme/acme.json}

USAGE="$(basename "$0") <path to acme> <destination cert directory>"

# Platform variations
case "$(uname)" in
	'Linux')
		# On Linux, -d should always work. --decode does not work with Alpine's busybox-binary
		CMD_DECODE_BASE64="base64 -d"
		;;
	*)
		# Max OS-X supports --decode and -D, but --decode may be supported by other platforms as well.
		CMD_DECODE_BASE64="base64 --decode"
		;;
esac

inform() {
	local script=$(basename $0)
	local level=$1
	shift
	# Assume interactive if we have stdout open and print usage message if needed.
	if [ -t 1 ]; then
		echo "$@"
		case $level in
			emerg|alert|crit|err|warning) printf "\n${USAGE}" ;;
		esac
	else
		# If we have /dev/log socket send message to logger otherwise to stdout.
		if [ -S /dev/log ]; then
			logger -t "${script}[${$}]" -p "auth.$level" "$@"
		else
			echo "${script}[${$}]: $@"
		fi
	fi
}

# Allow us to exit on a missing jq binary
exit_jq() {
	inform warn "You must have the binary jq to use this."
	exit 1
}

bad_acme() {
	inform warn "There was a problem parsing your acme.json file."
	exit 2
}

#if [ $# -ne 2 ]; then
#	echo "
#Insufficient number of parameters.
#
#${USAGE}" >&2
#	exit 1
#fi

# when called by inotifyd the first argument is the single character 
# event desciptior, lets drop it
[ ${#1} -eq 1 ] && shift

readonly acmefile="${1-$ACME_FILE}"
readonly certdir="${2-$DOCKER_ACME_SSL_DIR}"

if [ ! -r "${acmefile}" ]; then
	inform warn "There was a problem reading from (${acmefile}). We need to read this file to explode the JSON bundle... exiting."
	exit 2
fi


if [ ! -d "${certdir}" ]; then
	inform warn "Path ${certdir} does not seem to be a directory. We need a directory in which to explode the JSON bundle... exiting."
	exit 4
fi

jq=$(command -v jq) || exit_jq

priv=$(${jq} -e -r '.Account.PrivateKey' "${acmefile}") || bad_acme

if [ ! -n "${priv}" ]; then
	inform warn "There didn't seem to be a private key in ${acmefile}. Please ensure that there is a key in this file and try again."
	exit 8
fi

# If they do not exist, create the needed subdirectories for our assets
# and place each in a variable for later use, normalizing the path
mkdir -p "${certdir}/certs" "${certdir}/private"

pdir="${certdir}/private/"
cdir="${certdir}/certs/"

# Save the existing umask, change the default mode to 600, then
# after writing the private key switch it back to the default
oldumask=$(umask)
umask 177
trap 'umask ${oldumask}' EXIT

# traefik stores the private key in stripped base64 format but the certificates
# bundled as a base64 object without stripping headers.  This normalizes the
# headers and formatting.
#
# In testing this out it was a balance between the following mechanisms:
# gawk:
#  echo ${priv} | awk 'BEGIN {print "-----BEGIN RSA PRIVATE KEY-----"}
#     {gsub(/.{64}/,"&\n")}1
#     END {print "-----END RSA PRIVATE KEY-----"}' > "${pdir}/letsencrypt.key"
#
# openssl:
# echo -e "-----BEGIN RSA PRIVATE KEY-----\n${priv}\n-----END RSA PRIVATE KEY-----" \
#   | openssl rsa -inform pem -out "${pdir}/letsencrypt.key"
#
# and sed:
# echo "-----BEGIN RSA PRIVATE KEY-----" > "${pdir}/letsencrypt.key"
# echo ${priv} | sed -E 's/(.{64})/\1\n/g' >> "${pdir}/letsencrypt.key"
# sed -i '$ d' "${pdir}/letsencrypt.key"
# echo "-----END RSA PRIVATE KEY-----" >> "${pdir}/letsencrypt.key"
# openssl rsa -noout -in "${pdir}/letsencrypt.key" -check  # To check if the key is valid

# In the end, openssl was chosen because most users will need this script
# *because* of openssl combined with the fact that it will refuse to write the
# key if it does not parse out correctly. The other mechanisms were left as
# comments so that the user can choose the mechanism most appropriate to them.
printf -- \
	"-----BEGIN RSA PRIVATE KEY-----\n%s\n-----END RSA PRIVATE KEY-----\n" \
	${priv} | fold -w 65 | \
	openssl rsa -inform pem -out "${pdir}/letsencrypt.key" 2>/dev/null

# Process the certificates for each of the domains in acme.json
domains=$(jq -r '.Certificates[].Domain.Main' ${acmefile}) || bad_acme

inform notice "Extracting private keys and cert bundles in ${acmefile}"
inform debug  "Extracting private key and cert bundle for domains" ${domains}

for domain in $domains; do
	# Traefik stores a cert bundle for each domain.  Within this cert
	# bundle there is both proper the certificate and the Let's Encrypt CA
	cert=$(jq -e -r --arg domain "$domain" '.Certificates[] |
		select (.Domain.Main == $domain )| .Certificate' ${acmefile}) || bad_acme
	echo "${cert}" | ${CMD_DECODE_BASE64} > "${cdir}/${domain}.crt"

	key=$(jq -e -r --arg domain "$domain" '.Certificates[] |
		select (.Domain.Main == $domain )| .Key' ${acmefile}) || bad_acme
	echo "${key}" | ${CMD_DECODE_BASE64} > "${pdir}/${domain}.key"
done
