# inkspool

`inkspool` is a small C++17 document-capsule reader for offline page snapshots.
It parses a deterministic text/binary-friendly `.isp` container, validates
references, builds cross-section indexes, and replays page operations into a
render plan that inspection tools can print without needing a graphics stack.

The repository is intentionally self-contained: it uses only the C++17 standard
library, ships its own parser and validation layers, and does not fetch code or
data during the build.

## Components

- `include/inkspool/` contains the public parser, diagnostics, document model,
  validation, indexing, replay, query, and serialization interfaces.
- `src/` implements the lexer, structured parser, semantic validator, reference
  indexes, page replay engine, query scanner, checksum helpers, and normalized
  writer.
- `tools/inkspool_inspect.cpp` is a useful CLI for validation, summaries,
  normalized dumps, and render-plan inspection.
- `tests/unit_tests.cpp` exercises successful parsing, semantic errors, index
  construction, writer normalization, and replay behavior.
- `fuzz/spool_document_fuzzer.cpp` drives full parse, validate, index, replay,
  query, checksum, and serialization paths from raw bytes.
- `fuzz/spool_query_fuzzer.cpp` drives parse, validate, reference indexing,
  query scanning, and normalized writer paths from raw bytes.
- `fuzz/corpus/` provides per-target seed corpora in a Fenrir-recognized
  location.
- `.clusterfuzzlite/` contains the ClusterFuzzLite build entrypoint and target
  list.

## Build Instructions

Local build with a C++17 compiler:

```bash
make
```

Build only the inspection tool:

```bash
make bin/inkspool_inspect
```

Build with sanitizers when using Clang:

```bash
make clean
CXX=clang++ CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -Iinclude -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" make
```

## Test Instructions

```bash
make test
```

The tests are deterministic and do not require network access, credentials,
absolute local paths, or prompts.

## Fuzzing Instructions

ClusterFuzzLite runs `.clusterfuzzlite/build.sh`. For a local Clang/libFuzzer
build, one equivalent command is:

```bash
rm -rf out && mkdir -p out
SRC="$PWD" OUT="$PWD/out" CXX=clang++ \
  CXXFLAGS="-std=c++17 -I$PWD/include -O1 -g -fsanitize=address,undefined,fuzzer-no-link -fno-omit-frame-pointer" \
  LIB_FUZZING_ENGINE="-fsanitize=fuzzer" \
  .clusterfuzzlite/build.sh
```

Then run either target:

```bash
out/spool_document_fuzzer fuzz/corpus/spool_document_fuzzer
out/spool_query_fuzzer fuzz/corpus/spool_query_fuzzer
```

## Seed Corpus Explanation

The per-target corpora under `fuzz/corpus/` contain valid and near-valid
`INKSPOOL/1` capsules. Seeds include string tables, style inheritance, page
definitions, xrefs, nested state operations, deferred anchors, and normalized
writer-friendly examples. The crashing proof of concept is not in the corpus.

`fuzz/dictionary.txt` includes format magic, section names, record tokens,
operation keywords, delimiters, color literals, blend names, and common field
names useful for mutating deeper parser and replay states.

## Fenrir Readiness Checklist

- [x] Repository is designed for a private GitHub submission.
- [x] Source is original and not copied from a public repository.
- [x] Full clean-checkout source is present.
- [x] C++17 code is split across many headers and source files.
- [x] Build is deterministic and non-interactive.
- [x] Build uses no credentials, prompts, absolute local paths, or network
  services.
- [x] `.clusterfuzzlite/build.sh` sits at repository root.
- [x] `build.sh` builds every fuzz target into `$OUT`.
- [x] `.clusterfuzzlite/project.yaml` lists all fuzz targets.
- [x] Multiple connected libFuzzer harnesses call real project code.
- [x] Harnesses accept raw bytes and drive deep parse/index/replay paths.
- [x] Seed corpus is placed under `fuzz/corpus/<target>/`.
- [x] Dictionary is placed at `fuzz/dictionary.txt`.
- [x] The known crashing PoC is kept outside the repository.

## Manual Review Checklist

- Confirm the final GitHub repository is private.
- Confirm the repository is not a fork and has no copied public history.
- Review all source for maintainability and intended behavior.
- Confirm `.clusterfuzzlite/build.sh` has executable permissions in Git.
- Rebuild from a clean checkout before submission.
- Re-run tests and both fuzz targets locally with sanitizers.
- Re-run the external PoC against every built target.
- Confirm the PoC is the exact raw input file, not a generator script.
- Confirm no crashing PoC was accidentally added to the seed corpus.
- Confirm task description after patching is concise and does not reveal fix
  instructions.

Reminder: the final GitHub repository submitted to Fenrir must be private,
original, and primarily human-written.

