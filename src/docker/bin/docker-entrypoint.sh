#!/usr/bin/env sh
# set -x
#
# This script need to run as PID 1 allowing it to receive signals from docker
#
# Usage: add the folowing lines in Dockerfile
# ENTRYPOINT ["docker-entrypoint.sh"]
# CMD runsvdir -P ${SVDIR}
#

#
# Variables
#
DOCKER_ENTRY_DIR=${DOCKER_ENTRY_DIR-/etc/docker/entry.d}
DOCKER_EXIT_DIR=${DOCKER_EXIT_DIR-/etc/docker/exit.d}
SVDIR=${SVDIR-/etc/service}

#
# Source common functions.
#
. docker-common.sh
. docker-config.sh

#
# Functions
#

#
# run_parts dir
# Read and execute commands from files in the _current_ shell environment
#
run_parts() {
	for file in $(find $1 -type f -executable 2>/dev/null|sort); do
		dc_log 7 run_parts: executing $file
		. $file
	done
}

#
# If the service is running, send it the TERM signal, and the CONT signal.
# If both files ./run and ./finish exits, execute ./finish.
# After it stops, do not restart the service.
#
sv_down() { sv down ${SVDIR}/* ;}

#
# SIGTERM handler
# docker stop first sends SIGTERM, and after a grace period, SIGKILL.
# use exit code 143 = 128 + 15 -- SIGTERM
#
term_trap() {
	dc_log 4 "Got SIGTERM, so shutting down."
	run_parts "$DOCKER_EXIT_DIR"
	sv_down
	exit 143
}


#
# Stage 0) Register signal handlers and redirect stderr
#

exec 2>&1
trap 'kill $!; term_trap' SIGTERM

#
# Stage 1) run all entry scripts in $DOCKER_ENTRY_DIR
#

run_parts "$DOCKER_ENTRY_DIR"

#
# Stage 2) run provided arguments in the background
# Start services with: runsvdir -P ${SVDIR}
#

"$@" &

#
# Stage 3) wait forever so we can catch the SIGTERM
#
while true; do
	tail -f /dev/null & wait $!
done
