#!/usr/bin/env bash
set -euo pipefail

HOST_PORT=${HOST_PORT:-5555}
exec /usr/bin/socat STDIO TCP:127.0.0.1:"$HOST_PORT"

