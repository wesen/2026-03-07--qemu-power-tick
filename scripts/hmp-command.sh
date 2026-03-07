#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 MONITOR_SOCKET COMMAND..." >&2
  exit 1
fi

MONITOR_SOCKET=$1
shift
COMMAND="$*"

printf '%s\n' "$COMMAND" | socat - UNIX-CONNECT:"$MONITOR_SOCKET"
