#!/bin/sh
#
# switches
# -q 		send stdout and stderr to /dev/null
# -n <name>	use this name instead of basename $1
# -l 		activate svlogd (NOT IMPLEMENTED YET)
#
#

# use /etc/service if $DOCKER_RUNSV_DIR not already defined
DOCKER_RUNSV_DIR=${DOCKER_RUNSV_DIR-/etc/service}
DOCKER_SVLOG_DIR=${DOCKER_SVLOG_DIR-/var/log/sv}

#
# Define helpers
#

init_service() {
	local redirstd=
	local runit_name cmd runit_dir svlog_dir use_log
	case "$1" in
		-q|--quiet)
			redirstd="exec >/dev/null"
			shift
			;;
		-n|--name)
			shift
			runit_name="$1"
			shift
			;;
		-l|--log)
			use_log="yes"
			shift
			;;
		-*|--*)
			echo "unknown switch in $@"
			exit
			;;
	esac
	cmd=$(which "$1")
	runit_name=${runit_name-$(base_name $1)}
	runit_dir=$DOCKER_RUNSV_DIR/$runit_name
	svlog_dir=$DOCKER_SVLOG_DIR/$runit_name
	shift
	if [ ! -z "$cmd" ]; then
		mkdir -p $runit_dir
		cat <<-! > $runit_dir/run
			#!/bin/sh -e
			exec 2>&1
			$redirstd
			exec $cmd $@
		!
		chmod +x $runit_dir/run
		if [ -n "$use_log" ]; then
			mkdir -p $runit_dir/log $svlog_dir
			cat <<-! > $runit_dir/log/run
				#!/bin/sh
				exec svlogd -tt $svlog_dir
			!
			chmod +x $runit_dir/log/run
		fi
	fi
	}

down_service() {
	local cmd=$1
	touch $DOCKER_RUNSV_DIR/$cmd/down
	}

base_name() { local base=${1##*/}; echo ${base%%.*} ;}

#
# run
#

for cmd in "$@" ; do
	init_service $cmd
done
