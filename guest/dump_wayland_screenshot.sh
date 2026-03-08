#!/bin/busybox sh
set -eu

TAG=${1:-shot}
OUT_DIR=${2:-/var/log/guestshots}
LOG_PATH="$OUT_DIR/weston-screenshooter-$TAG.log"

mkdir -p "$OUT_DIR"
find "$OUT_DIR" /tmp /root /run -name 'wayland-screenshot-*.png' -type f -delete 2>/dev/null || true

export XDG_PICTURES_DIR="$OUT_DIR"

if ! (
  cd "$OUT_DIR"
  timeout 10 /usr/bin/weston-screenshooter >"$LOG_PATH" 2>&1
); then
  echo "@@GUESTSHOT_ERROR tag=$TAG reason=weston-screenshooter-failed"
  [ -f "$LOG_PATH" ] && sed -n '1,40p' "$LOG_PATH"
  exit 1
fi

SHOT=
i=0
while [ $i -lt 20 ]; do
  SHOT=$(find "$OUT_DIR" /tmp /root /run /home -name 'wayland-screenshot-*.png' -type f 2>/dev/null | head -n 1 || true)
  [ -n "$SHOT" ] && break
  i=$((i + 1))
  sleep 0.1
done

if [ -z "$SHOT" ] || [ ! -f "$SHOT" ]; then
  echo "@@GUESTSHOT_ERROR tag=$TAG reason=missing-output"
  [ -f "$LOG_PATH" ] && sed -n '1,40p' "$LOG_PATH"
  exit 1
fi

SIZE=$(wc -c <"$SHOT" | tr -d ' ')
SHA=$(sha256sum "$SHOT" | awk '{print $1}')
echo "@@GUESTSHOT tag=$TAG path=$SHOT size=$SIZE sha256=$SHA"
echo "@@GUESTSHOT_BEGIN tag=$TAG"
base64 "$SHOT"
echo "@@GUESTSHOT_END tag=$TAG"
