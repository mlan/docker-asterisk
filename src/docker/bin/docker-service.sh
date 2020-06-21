#!/bin/sh
#
# docker-service.sh
#
source docker-common.sh

# use /etc/service if $SVDIR not already defined
SVDIR=${SVDIR-/etc/service}
DOCKER_SVLOG_DIR=${DOCKER_SVLOG_DIR-/var/log/sv}
DOCKER_RUN_DIR=${DOCKER_RUN_DIR-/var/run}

#
# Define helpers
#
usage() {
	cat <<-!cat
		 NAME
		  docker-service.sh

		 SYNOPSIS
		  docker-service.sh [-d] [-f] [-h] [-l] [-n name] [-s file] [-q] command [args]

		 OPTIONS
		  -d       default down
		  -f       remove lingering pid file at start up
		  -h       print this text and exit
		  -l       activate logging (svlogd)
		  -n name  use this name instead of command
		  -s file  source file
		  -q       send stdout and stderr to /dev/null

		 EXAMPLES
		  docker-service.sh "kopano-dagent -l" "-d kopano-grapi serve"
		  "-q -s /etc/apache2/envvars apache2 -DFOREGROUND -DNO_DETACH -k start"

	!cat
}

base_name() { local base=${1##*/}; echo ${base%%.*} ;}

pid_name() {
	local dir_name=${1%%-*}
	local pid_name=${1##*-}
	echo "${DOCKER_RUN_DIR}/${dir_name}/${pid_name}.pid"
}

add_opt() {
	if [ -z "$options" ]; then
		options=$1
	else
		options="$options,$1"
	fi
}

#
# Define main function
#

init_service() {
	local redirstd=
	local clearpid=
	local sourcefile=
	local sv_name cmd runsv_dir svlog_dir sv_log sv_down sv_force options
	while getopts ":dfhln:s:q" opts; do
		case "${opts}" in
			d) sv_down="down"; add_opt "down";;
			f) sv_force="force"; add_opt "force";;
			h) usage; exit;;
			l) sv_log="log"; add_opt "log";;
			n) sv_name="${OPTARG}"; add_opt "name";;
			s) sourcefile=". ${OPTARG}"; add_opt "source";;
			q) redirstd="exec >/dev/null"; add_opt "quiet";;
		esac
	done
	shift $((OPTIND -1))
	cmd=$(which "$1")
	sv_name=${sv_name-$(base_name $1)}
	runsv_dir=$SVDIR/$sv_name
	svlog_dir=$DOCKER_SVLOG_DIR/$sv_name
	if [ -n "$sv_force" ]; then
		forcepid="$(echo rm -f $(pid_name $sv_name)*)"
	fi
	shift
	if [ ! -z "$cmd" ]; then
		dc_log 5 "Setting up ($sv_name) options ($options) args ($@)"
		mkdir -p $runsv_dir
		cat <<-!cat > $runsv_dir/run
			#!/bin/sh -e
			exec 2>&1
			$forcepid
			$redirstd
			$sourcefile
			exec $cmd $@
		!cat
		chmod +x $runsv_dir/run
		if [ -n "$sv_down" ]; then
			touch $runsv_dir/down
		fi
		if [ -n "$sv_log" ]; then
			mkdir -p $runsv_dir/log $svlog_dir
			cat <<-!cat > $runsv_dir/log/run
				#!/bin/sh
				exec svlogd -tt $svlog_dir
			!cat
			chmod +x $runsv_dir/log/run
		fi
	fi
	}

#
# run
#

for cmd in "$@" ; do
	init_service $cmd
done
