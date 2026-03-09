#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
usage: run_phase4_headless_smoke.sh [--payload-dir DIR] [--results-dir DIR]

Runs a host-side content_shell headless smoke test against a temporary local
HTML page that closes itself after load. This validates the built binary and
payload before the phase-4 DRM guest path is exercised.
EOF
  exit 1
}

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
PAYLOAD_DIR=${PAYLOAD_DIR:-"$HOME/chromium/src/out/Phase4DRM"}
RESULTS_DIR=${RESULTS_DIR:-"$REPO_ROOT/results-phase4-headless1"}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --payload-dir)
      PAYLOAD_DIR=$2
      shift 2
      ;;
    --results-dir)
      RESULTS_DIR=$2
      shift 2
      ;;
    *)
      usage
      ;;
  esac
done

PAYLOAD_DIR=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$PAYLOAD_DIR")
RESULTS_DIR=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$RESULTS_DIR")

mkdir -p "$RESULTS_DIR"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT
PAGE="$TMPDIR/headless-smoke.html"

cat >"$PAGE" <<'EOF'
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>phase4-headless-smoke</title>
  <script>
    window.addEventListener('load', () => {
      document.body.textContent = 'phase4 headless smoke ok';
      setTimeout(() => { window.close(); }, 200);
    });
  </script>
</head>
<body>loading</body>
</html>
EOF

STDOUT_LOG="$RESULTS_DIR/stdout.log"
STDERR_LOG="$RESULTS_DIR/stderr.log"
META_JSON="$RESULTS_DIR/meta.json"
PROFILE_DIR="$TMPDIR/profile"

set +e
(
  cd "$PAYLOAD_DIR"
  timeout 20s ./content_shell \
    --no-sandbox \
    --disable-system-font-check \
    --ozone-platform=headless \
    --user-data-dir="$PROFILE_DIR" \
    "file://$PAGE"
) >"$STDOUT_LOG" 2>"$STDERR_LOG"
RC=$?
set -e

python3 - <<'PY' "$META_JSON" "$PAYLOAD_DIR" "$PAGE" "$RC"
import json, pathlib, sys
path = pathlib.Path(sys.argv[1])
path.write_text(json.dumps({
    "payload_dir": sys.argv[2],
    "page": sys.argv[3],
    "returncode": int(sys.argv[4]),
}, indent=2) + "\n", encoding="utf-8")
PY

echo "headless smoke rc=$RC"
echo "results_dir=$RESULTS_DIR"
exit "$RC"
