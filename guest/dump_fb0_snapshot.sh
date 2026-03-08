#!/bin/busybox sh
set -eu

TAG=${1:-snapshot}

sanitize() {
  tr ' \t\n' '_' | tr -cd '[:alnum:]_./,:+-'
}

read_file() {
  if [ -r "$1" ]; then
    cat "$1" 2>/dev/null | sanitize
  else
    printf "missing"
  fi
}

MODE_LINE=$(head -n 1 /sys/class/graphics/fb0/modes 2>/dev/null || true)
VIRTUAL_SIZE=$(cat /sys/class/graphics/fb0/virtual_size 2>/dev/null || true)
BPP=$(cat /sys/class/graphics/fb0/bits_per_pixel 2>/dev/null || true)
STRIDE=$(cat /sys/class/graphics/fb0/stride 2>/dev/null || true)
NAME=$(read_file /sys/class/graphics/fb0/name)
PROC_FB=$(cat /proc/fb 2>/dev/null | sanitize || printf "missing")

if [ ! -c /dev/fb0 ]; then
  echo "@@FBSHOT_ERROR tag=$TAG reason=missing-device device=/dev/fb0"
  exit 1
fi

case "$VIRTUAL_SIZE" in
  *,*)
    VIRTUAL_WIDTH=${VIRTUAL_SIZE%%,*}
    VIRTUAL_HEIGHT=${VIRTUAL_SIZE##*,}
    ;;
  *)
    echo "@@FBSHOT_ERROR tag=$TAG reason=missing-virtual-size value=$(printf '%s' "$VIRTUAL_SIZE" | sanitize)"
    exit 1
    ;;
esac

VISIBLE_WIDTH=$VIRTUAL_WIDTH
VISIBLE_HEIGHT=$VIRTUAL_HEIGHT
case "$MODE_LINE" in
  *:*)
    DIMS=${MODE_LINE#*:}
    DIMS=${DIMS%%[!0-9x]*}
    case "$DIMS" in
      *x*)
        VISIBLE_WIDTH=${DIMS%%x*}
        VISIBLE_HEIGHT=${DIMS##*x}
        ;;
    esac
    ;;
esac

if [ -z "$BPP" ] || [ -z "$STRIDE" ] || [ -z "$VISIBLE_HEIGHT" ]; then
  echo "@@FBSHOT_ERROR tag=$TAG reason=missing-metadata bpp=$(printf '%s' "$BPP" | sanitize) stride=$(printf '%s' "$STRIDE" | sanitize) visible_height=$(printf '%s' "$VISIBLE_HEIGHT" | sanitize)"
  exit 1
fi

TOTAL_BYTES=$((STRIDE * VISIBLE_HEIGHT))
RAW="/tmp/fbshot-${TAG}.raw"
RAW_GZ="${RAW}.gz"

dd if=/dev/fb0 of="$RAW" bs="$STRIDE" count="$VISIBLE_HEIGHT" >/dev/null 2>&1
gzip -c -1 "$RAW" >"$RAW_GZ"
RAW_SIZE=$(wc -c <"$RAW" | tr -d ' ')
GZ_SIZE=$(wc -c <"$RAW_GZ" | tr -d ' ')
SHA_RAW=$(sha256sum "$RAW" | awk '{print $1}')

echo "@@FBSHOT tag=$TAG name=$NAME proc_fb=$PROC_FB virtual_width=$VIRTUAL_WIDTH virtual_height=$VIRTUAL_HEIGHT visible_width=$VISIBLE_WIDTH visible_height=$VISIBLE_HEIGHT bpp=$BPP stride=$STRIDE total_bytes=$TOTAL_BYTES raw_size=$RAW_SIZE gzip_size=$GZ_SIZE sha256_raw=$SHA_RAW mode=$(printf '%s' "$MODE_LINE" | sanitize) compression=gzip"
echo "@@FBSHOT_BEGIN tag=$TAG"
base64 "$RAW_GZ"
echo "@@FBSHOT_END tag=$TAG"

rm -f "$RAW" "$RAW_GZ"
