#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SKIRK_BIN="$BUILD_DIR/skirk"

log() { printf "\033[1;34m==>\033[0m %s\n" "$*"; }
fail() { printf "\033[1;31mFAIL:\033[0m %s\n" "$*"; exit 1; }

# 1) Build
log "Building skirk..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. >/dev/null
make -j >/dev/null
[ -x "$SKIRK_BIN" ] || fail "skirk binary not found"

# 2) Temp repo
TMP_REPO="$(mktemp -d /tmp/skirk-e2e.XXXXXX)"
log "Using temp repo: $TMP_REPO"
cd "$TMP_REPO"

# 3) Init
log "Init repo"
"$SKIRK_BIN" init .
[ -d ".mygit" ] || fail ".mygit not created"

# 4) Create a file
echo "hello world" > hello.txt

# 5) Add
log "Add hello.txt"
"$SKIRK_BIN" add hello.txt
[ -f ".mygit/index" ] || fail "index not written after add"

# 6) Commit
MSG="initial commit"
log "Commit with message: $MSG"
"$SKIRK_BIN" commit -m "$MSG"

# 7) Resolve HEAD commit SHA
resolve_head_sha() {
  local head_file=".mygit/HEAD"
  [ -f "$head_file" ] || return 1
  local head_content
  head_content="$(cat "$head_file")"
  if [[ "$head_content" == ref:\ refs/heads/* ]]; then
    local ref="${head_content#ref: }"
    ref="${ref//$'\n'/}"
    if [ -f ".mygit/$ref" ]; then
      tr -d '\n' < ".mygit/$ref"
      return 0
    fi
  else
    tr -d '\n' < "$head_file"
    return 0
  fi
  return 1
}

HEAD_SHA="$(resolve_head_sha || true)"
[ ${#HEAD_SHA} -eq 40 ] || fail "HEAD does not point to a 40-hex commit (got: $HEAD_SHA)"

# 8) ls-tree HEAD (should list hello.txt)
log "List tree for HEAD"
LS_OUT="$("$SKIRK_BIN" ls-tree HEAD || true)"
echo "$LS_OUT" | grep -q "hello.txt" || fail "ls-tree HEAD did not list hello.txt"

# 9) show-ref (should list refs or HEAD)
log "Show refs"
SHOW_REF_OUT="$("$SKIRK_BIN" show-ref || true)"
echo "$SHOW_REF_OUT" | grep -E -q 'refs/heads/|refs/tags/|HEAD' || log "No refs listed (detached HEAD?), continuing"

log "All steps passed."
echo "Temp repo kept at: $TMP_REPO"
echo "Set KEEP=0 to auto-clean."