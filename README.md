# Skirk — a minimal Git‑like VCS in C++17

Skirk is a learning/teaching implementation of a Git‑like version control system in modern C++17. It stores content as compressed objects addressed by SHA‑1, organizes files into trees, tracks history with commits, and uses refs for branches/tags. Repository metadata lives under a `.mygit` directory in your worktree (intentionally not `.git`).

## Features

- Content‑addressed object store (SHA‑1) with types: blob, tree, commit, tag
- Recursive trees built from a flat index (bottom‑up aggregation)
- Refs with indirection (e.g. `HEAD` → `refs/heads/main`)
- Lightweight and annotated tags
- Name resolution: `HEAD`, branches, tags, SHA or prefix
- Basic index operations: add, rm, write/read index
- Commit creation and branch/HEAD updates

## Repo layout

- `src/commands/` — command handlers (init, hash-file, cat-file, ls-tree, checkout, show-ref, tag, commit, add, rm, status)
- `src/utils/` — core utilities: object model, object I/O, refs, index/commit plumbing
- `src/cli/bridge.cpp` — CLI argument parsing and dispatch
- `.mygit/` — repository metadata (created by `init` at runtime)

## Build

Skirk uses CMake.

```bash
mkdir -p build
cd build
cmake ..
make -j
```

This produces the `skirk` binary under `build/`.

## Quick start

Initialize a repo in the current directory:

```bash
./build/skirk init .
```

Add and commit a file:

```bash
echo "hello" > hello.txt
./build/skirk add hello.txt
./build/skirk commit -m "initial commit"
```

List the tree at `HEAD` (recursively):

```bash
./build/skirk ls-tree -r HEAD
```

Create and list tags:

```bash
# lightweight tag pointing to HEAD
./build/skirk tag v0.1

# annotated tag object
./build/skirk tag -a v0.1-ann

# list tags
./build/skirk tag
```

Inspect an object by SHA:

```bash
./build/skirk cat-file -p <sha>
```

## Commands (overview)

- `init <path>` — create `.mygit` at path; writes `HEAD`, `config`, `refs`, and object store dirs
- `hash-file [-w] -t <type> <path>` — hash file and optionally store object
- `cat-file -p <sha>` — print parsed object content
- `ls-tree [-r] <treeish>` — list entries for a tree/commit/tag/sha
- `checkout <treeish> <dest>` — write a snapshot to dest (dest must exist and be empty)
- `show-ref` — list refs under `.mygit/refs/**` (and `HEAD`)
- `tag [-a] [<name> [<target>]]` — list or create tags (annotated with `-a`)
- `add <path...>` — add file(s) to the index (files only)
- `rm [-f] <path...>` — remove file(s) from index (and optionally worktree)
- `commit -m <msg>` — create a commit from index, update `HEAD`/current branch
- `status` — basic status of branch and staged entries

Note: Index/commit/status logic lives in `src/utils/index.cpp`.

## Debug logging

Many modules emit debug logs to stderr with a `[DEBUG][TAG]` prefix (e.g., `INDEX_LOAD`, `WRITE_INDEX`, `ADD`, `RM`, `commit_cmd`, `hashfile`). Redirect stderr to capture traces:

```bash
./build/skirk add hello.txt 2> debug.log
```

## Troubleshooting

- “run init first” — Run `./build/skirk init .` to create `.mygit/`.
- “not a regular file” — `add`/`hash-file` accept files, not directories. Pass explicit files.
- “could not resolve treeish” — Use a valid name: `HEAD`, branch, tag, full SHA, or unambiguous prefix.
- Build/link errors — Ensure `CMakeLists.txt` includes all required sources (`commands/show-ref.cpp`, `commands/tag.cpp`, `utils/refWorkers.cpp`, `utils/index.cpp`) and links ZLIB. Re-run CMake after edits.

## End‑to‑end test

An example script exists at `tests/e2e_commit.sh` to exercise init → add → commit → ls-tree → show-ref:

```bash
chmod +x tests/e2e_commit.sh
./tests/e2e_commit.sh
```

## Notes / next steps

- Argument shapes mirror Git where practical but remain minimal by design.
- Improve worktree vs index status detection (modified/untracked) for `status`.
- Extend tagger identity/message collection for annotated tags.
- Add unit tests for object parsing and name resolution.
