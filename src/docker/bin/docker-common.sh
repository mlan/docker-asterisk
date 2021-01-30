#!/bin/sh
#
# docker-common.sh
#
# Defines common functions. Source this file from other scripts.
#
DOCKER_LOGLEVEL=${DOCKER_LOGLEVEL-5}
DOCKER_LOGENTRY=${DOCKER_LOGENTRY-docker-entrypoint.sh}
DOCKER_LOGUSAGE=${DOCKER_LOGUSAGE-usage}

#
# Write messages to console if interactive or syslog if not.
# Usage: inform priority message
# The priority may be specified numerically or as a facility.level pair.
# Example user.notice, or 1.6 level is one of:
# 0|emerg|1|alert|2|crit|3|err|4|warning|5|notice|6|info|7|debug
#
dc_log() {
	local script=$(basename $0)
	local stamp="$(dc_log_stamp)"
	local prio=$1
	local level=${prio#*.}
	local logtag="${script}[${$}]"
	local ttytag="$(dc_log_stamp)$(dc_log_tag $level $logtag):"
	shift
	# Assume interactive if we have stdout open and print usage message if needed.
	if [ -t 1 ]; then
		echo "$@"
		case "$level" in
			0|emerg|1|alert|2|crit|3|err) $DOCKER_LOGUSAGE 2>/dev/null ;;
		esac
	else
		# If we have /dev/log socket send message to logger otherwise to stdout.
		if [ -S /dev/log ]; then
			logger -t "$logtag" -p "$prio" "$@"
		else
			if dc_log_level "$level"; then
				echo "$ttytag $@"
			fi
		fi
	fi
}

#
# Color log output. Used if the syslogd daemon is not running.
#
dc_log_tag() {
	local level=$1
	local string=$2
	local c l
	case $level in
		0|emerg)   c=91; l=EMERG ;;
		1|alert)   c=91; l=ALERT ;;
		2|crit)    c=91; l=CRIT ;;
		3|err)     c=91; l=ERROR ;;
		4|warning) c=93; l=WARN ;;
		5|notice)  c=92; l=NOTE ;;
		6|info)    c=92; l=INFO ;;
		7|debug)   c=92; l=DEBUG ;;
	esac
	printf "\e[%sm%s %s\e[0m\n" $c $string $l
}

#
# Use $DOCKER_LOGLEVEL during image build phase. Assume we are in build phase if
# $DOCKER_LOGENTRY is not running.
#
dc_log_level() {
	local level=$1
	if pidof $DOCKER_LOGENTRY >/dev/null; then
		[ "$level" -le "$SYSLOG_LEVEL" ]
	else
		[ "$level" -le "$DOCKER_LOGLEVEL" ]
	fi
}

#
# Don't add time stamp during image build phase. Assume we are in build phase if
# $DOCKER_LOGENTRY is not running.
#
dc_log_stamp() {
	if grep -q $DOCKER_LOGENTRY /proc/1/cmdline; then
		date +'%b %e %X '
	fi
}

#
# Tests if command is in the path
#
dc_is_command() { [ -x "$(command -v $1)" ] ;}

#
# Tests if pkgs are installed
#
dc_is_installed() {
	if dc_is_command apk; then
		ver_cmd="apk -e info"
	elif dc_is_command dpkg; then
		ver_cmd="dpkg -s"
	else
		dc_log 5 "No package manager found among: apk dpkg"
	fi
	for cmd in $@; do
		$ver_cmd $cmd > /dev/null 2>&1 || return 1
	done
}

#
# Update loglevel
#
dc_update_loglevel() {
	loglevel=${1-$SYSLOG_LEVEL}
	if [ -n "$loglevel" ]; then
		dc_log 5 "Setting syslogd level=$loglevel."
		docker-service.sh "syslogd -nO- -l$loglevel $SYSLOG_OPTIONS"
		[ -n "$DOCKER_RUNFUNC" ] && sv restart syslogd
	fi
}

#
# Print package versions
#
dc_pkg_versions() {
	local pkgs="$@"
	local len=$(echo $pkgs | tr " " "\n" | wc -L)
	local ver ver_cmd sed_flt
	local os=$(sed -rn 's/PRETTY_NAME="(.*)"/\1/p' /etc/os-release)
	local kern=$(uname -r)
	local host=$(uname -n)
	dc_log 5 $host $os $kern
	if dc_is_command apk; then
		ver_cmd="apk info -s"
		sed_flt="s/.*-(.*)-.*/\1/p"
	elif dc_is_command dpkg; then
		ver_cmd="dpkg -s"
		sed_flt="s/Version: ([[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+).*/\1/p"
	else
		dc_log 5 "No package manager found among: apk dpkg"
	fi
	for pkg in $pkgs; do
		ver=$($ver_cmd $pkg 2> /dev/null | sed -rn "$sed_flt")
		if [ -n "$ver" ]; then
			printf "\t%-${len}s\t%s\n" $pkg $ver
		fi
	done
}
