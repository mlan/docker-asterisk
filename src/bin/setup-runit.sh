#!/bin/sh
#
# switches
# -q send stdout and stderr to /dev/null
# -l activate svlogd (NOT IMPLEMENTED YET)
#
#

# use /etc/service if $DOCKER_RUNIT_DIR not already defined
DOCKER_RUNIT_DIR=${DOCKER_RUNIT_DIR-/etc/service}
#docker_svlog_dir=${docker_svlog_dir-/var/log/sv}

#
# Define helpers
#

init_service() {
	local redirstd=
	case "$1" in
		-q|--quiet)
			redirstd="exec >/dev/null"
			shift
			;;
		-*|--*)
			echo "unknown switch in $@"
			exit
			;;
	esac
	local cmd="$1"
	local runit_dir=$DOCKER_RUNIT_DIR/${cmd##*/}
	local svlog_dir=$docker_svlog_dir/${cmd##*/}
	shift
	cmd=$(which $cmd)
	if [ ! -z "$cmd" ]; then
		mkdir -p $runit_dir
		cat <<-! > $runit_dir/run
			#!/bin/sh -e
			exec 2>&1
			$redirstd
			exec $cmd $@
		!
		chmod +x $runit_dir/run
		if [ -n "$docker_svlog_dir" ]; then
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
	touch $DOCKER_RUNIT_DIR/$cmd/down
	}

#
# run
#

for cmd in "$@" ; do
	init_service $cmd
done
