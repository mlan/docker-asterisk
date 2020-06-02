#!/usr/bin/env bash
# set -x
#
# This script need to run as PID 1 allowing it to receive signals from docker
#
# Usage: add the folowing lines in Dockerfile
# ENTRYPOINT ["entrypoint.sh"]
# CMD runsvdir -P ${DOCKER_RUNSV_DIR}
#

#
# Variables
#

DOCKER_ENTRY_DIR=${DOCKER_ENTRY_DIR-/etc/entrypoint.d}
DOCKER_EXIT_DIR=${DOCKER_EXIT_DIR-/etc/exitpoint.d}
DOCKER_RUNSV_DIR=${DOCKER_RUNSV_DIR-/etc/service}

#
# source common functions
#
source docker-common.sh

#
# Functions
#

#
# Run all executable scripts in entry directory
#
run_dir() {
	local rundir=${1}
	if [ -d "$rundir" ]; then
		run-parts "$rundir"
	fi
}

#
# If the service is running, send it the TERM signal, and the CONT signal.
# If both files ./run and ./finish exits, execute ./finish.
# After it stops, do not restart the service.
#
sv_down() { sv down ${DOCKER_RUNSV_DIR}/* ;}

#
# SIGTERM handler
# docker stop first sends SIGTERM, and after a grace period, SIGKILL.
# use exit code 143 = 128 + 15 -- SIGTERM
#
term_trap() {
	run_dir "$DOCKER_EXIT_DIR"
	sv_down
	exit 143
}


#
# Stage 0) Register signal handlers and redirect stderr
#

exec 2>&1
trap 'kill ${!}; term_trap' SIGTERM

#
# Stage 1) run all entry scripts in $DOCKER_ENTRY_DIR
#

run_dir "$DOCKER_ENTRY_DIR"

#
# Stage 2) run provided arguments in the background
# Start services with: runsvdir -P ${DOCKER_RUNSV_DIR}
#

"$@" &

#
# Stage 3) wait forever so we can catch the SIGTERM
#
while true; do
	tail -f /dev/null & wait ${!}
done
