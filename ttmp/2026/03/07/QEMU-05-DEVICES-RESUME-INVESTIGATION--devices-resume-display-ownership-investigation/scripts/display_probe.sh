#!/bin/busybox sh
set -eu

INTERVAL=${1:-1}
LOG_PATH=${2:-/var/log/display_probe.log}

sanitize() {
  tr ' \t\n' '_' | tr -cd '[:alnum:]_./:-'
}

read_file() {
  if [ -r "$1" ]; then
    cat "$1" 2>/dev/null | sanitize
  else
    printf "missing"
  fi
}

list_vtconsoles() {
  for path in /sys/class/vtconsole/vtcon*; do
    [ -d "$path" ] || continue
    name=$(read_file "$path/name")
    bind=$(read_file "$path/bind")
    printf "%s{name=%s,bind=%s} " "$(basename "$path")" "$name" "$bind"
  done
}

list_drm() {
  for path in /sys/class/drm/card*-*; do
    [ -d "$path" ] || continue
    status=$(read_file "$path/status")
    enabled=$(read_file "$path/enabled")
    dpms=$(read_file "$path/dpms")
    printf "%s{status=%s,enabled=%s,dpms=%s} " "$(basename "$path")" "$status" "$enabled" "$dpms"
  done
}

sample=0
while true; do
  sample=$((sample + 1))
  uptime=$(cut -d' ' -f1 /proc/uptime 2>/dev/null || printf "0")
  proc_fb=$(cat /proc/fb 2>/dev/null | sanitize || printf "missing")
  fb0_name=$(read_file /sys/class/graphics/fb0/name)
  printf "@@DISPLAY sample=%d uptime_s=%s fb0=%s proc_fb=%s vt=%s drm=%s\n" \
    "$sample" "$uptime" "$fb0_name" "$proc_fb" "$(list_vtconsoles | sanitize)" "$(list_drm | sanitize)" \
    >>"$LOG_PATH"
  sleep "$INTERVAL"
done
