#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
usage: bootstrap_chromium_checkout.sh [CHECKOUT_DIR]

Environment overrides:
  DEPOT_TOOLS_DIR   Where depot_tools should live (default: $HOME/depot_tools)
  CHECKOUT_DIR      Chromium checkout root (default: $HOME/chromium)
  FETCH_TARGET      fetch target (default: chromium)
  FETCH_FLAGS       Extra flags passed to fetch (default: --nohooks)
  SYNC_FLAGS        Extra flags passed to gclient sync (default: --nohooks --with_branch_heads --with_tags)

This helper is idempotent:
  - clones or updates depot_tools
  - creates a Chromium checkout if missing
  - otherwise runs gclient sync in the existing checkout
EOF
  exit 1
}

if [[ $# -gt 1 ]]; then
  usage
fi

DEPOT_TOOLS_DIR=${DEPOT_TOOLS_DIR:-"$HOME/depot_tools"}
CHECKOUT_DIR=${CHECKOUT_DIR:-"${1:-$HOME/chromium}"}
FETCH_TARGET=${FETCH_TARGET:-chromium}
FETCH_FLAGS=${FETCH_FLAGS:---nohooks}
SYNC_FLAGS=${SYNC_FLAGS:---nohooks --with_branch_heads --with_tags}

mkdir -p "$(dirname "$DEPOT_TOOLS_DIR")" "$(dirname "$CHECKOUT_DIR")"

if [[ ! -d "$DEPOT_TOOLS_DIR/.git" ]]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS_DIR"
else
  git -C "$DEPOT_TOOLS_DIR" pull --ff-only
fi

export PATH="$DEPOT_TOOLS_DIR:$PATH"

if [[ ! -x "$DEPOT_TOOLS_DIR/fetch" || ! -x "$DEPOT_TOOLS_DIR/gclient" ]]; then
  echo "depot_tools install is incomplete: fetch/gclient missing in $DEPOT_TOOLS_DIR" >&2
  exit 1
fi

if [[ ! -d "$CHECKOUT_DIR/.gclient" && ! -d "$CHECKOUT_DIR/src/.git" ]]; then
  mkdir -p "$CHECKOUT_DIR"
  (
    cd "$CHECKOUT_DIR"
    # fetch creates src/ and the gclient configuration for the chosen solution.
    fetch $FETCH_FLAGS "$FETCH_TARGET"
  )
else
  (
    cd "$CHECKOUT_DIR"
    gclient sync $SYNC_FLAGS
  )
fi

echo "depot_tools=$DEPOT_TOOLS_DIR"
echo "checkout=$CHECKOUT_DIR"
echo "src=$CHECKOUT_DIR/src"
