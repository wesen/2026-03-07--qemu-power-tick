#!/bin/busybox sh
set -eu

TAG=${1:-state}

sanitize() {
  tr ' \t\n' '_' | tr -cd '[:alnum:]_./,:+-'
}

if ! mountpoint -q /sys/kernel/debug 2>/dev/null; then
  mkdir -p /sys/kernel/debug
  mount -t debugfs debugfs /sys/kernel/debug 2>/dev/null || true
fi

BASE=
for path in /sys/kernel/debug/dri/*; do
  [ -d "$path" ] || continue
  case "$(basename "$path")" in
    [0-9]*)
      BASE=$path
      break
      ;;
  esac
done

if [ -z "$BASE" ]; then
  echo "@@DRMSTATE_ERROR tag=$TAG reason=missing-dri-debugfs"
  exit 1
fi

FILES=
for name in state framebuffer clients gem_names summary name; do
  if [ -r "$BASE/$name" ]; then
    if [ -n "$FILES" ]; then
      FILES="$FILES,$name"
    else
      FILES=$name
    fi
  fi
done

if [ -z "$FILES" ]; then
  echo "@@DRMSTATE_ERROR tag=$TAG reason=no-readable-files base=$(printf '%s' "$BASE" | sanitize)"
  exit 1
fi

TMP="/tmp/drmstate-${TAG}.txt"
TMP_GZ="${TMP}.gz"
: >"$TMP"

OLD_IFS=$IFS
IFS=,
for name in $FILES; do
  printf "===== FILE %s =====\n" "$name" >>"$TMP"
  cat "$BASE/$name" >>"$TMP" 2>/dev/null || true
  printf "\n===== END FILE %s =====\n" "$name" >>"$TMP"
done
IFS=$OLD_IFS

gzip -c -1 "$TMP" >"$TMP_GZ"
TXT_SIZE=$(wc -c <"$TMP" | tr -d ' ')
GZ_SIZE=$(wc -c <"$TMP_GZ" | tr -d ' ')
SHA_TXT=$(sha256sum "$TMP" | awk '{print $1}')
CARD=$(basename "$BASE")

echo "@@DRMSTATE tag=$TAG card=$CARD base=$(printf '%s' "$BASE" | sanitize) files=$FILES text_size=$TXT_SIZE gzip_size=$GZ_SIZE sha256_txt=$SHA_TXT compression=gzip"
echo "@@DRMSTATE_BEGIN tag=$TAG"
base64 "$TMP_GZ"
echo "@@DRMSTATE_END tag=$TAG"

rm -f "$TMP" "$TMP_GZ"
