#!/bin/sh -e

DOCKER_ENTRY_DIR=${DOCKER_ENTRY_DIR-/etc/entrypoint.d}

# redirect stderr
exec 2>&1

# run all entry scripts in $DOCKER_ENTRY_DIR
if [ -d "$DOCKER_ENTRY_DIR" ]; then
	run-parts "$DOCKER_ENTRY_DIR"
fi

# assume arguments are command with arguments to be executed
exec "$@"
