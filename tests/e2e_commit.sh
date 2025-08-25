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

# 2) Use build directory as parent so we can run relative commands like ./skirk and ../skirk
TMP_PARENT="$BUILD_DIR"
INIT_NAME="fn"
log "Using build dir as parent: $TMP_PARENT (worktree will be $INIT_NAME)"
cd "$TMP_PARENT"

# 3) Init (run from build dir using relative ./skirk to match local usage)
log "Running: ./skirk init $INIT_NAME force"
./skirk init "$INIT_NAME" force
[ -d "$INIT_NAME/.mygit" ] || fail ".mygit not created in $INIT_NAME"
cd "$INIT_NAME"

# 4) Create a file
echo "hello world" > hello.txt

# 5) Add (run using ../skirk since we're in the worktree under build)
log "Running: ../skirk add hello.txt"
../skirk add hello.txt
[ -f ".mygit/index" ] || fail "index not written after add"

# 6) Commit (use ../skirk)
MSG="initial commit"
log "Running: ../skirk commit -m \"$MSG\""
../skirk commit -m "$MSG"

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