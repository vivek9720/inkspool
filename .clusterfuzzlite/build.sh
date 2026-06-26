#!/bin/bash -eu

ROOT="${SRC:-$(pwd)}"
OUT_DIR="${OUT:?OUT must be set by ClusterFuzzLite}"
CXX_BIN="${CXX:-clang++}"
FUZZING_ENGINE="${LIB_FUZZING_ENGINE:-}"

COMMON_SRC=(
  "$ROOT/src/byte_view.cpp"
  "$ROOT/src/checksum.cpp"
  "$ROOT/src/diagnostic.cpp"
  "$ROOT/src/document.cpp"
  "$ROOT/src/format_writer.cpp"
  "$ROOT/src/indexer.cpp"
  "$ROOT/src/lexer.cpp"
  "$ROOT/src/page_script.cpp"
  "$ROOT/src/parser.cpp"
  "$ROOT/src/query.cpp"
  "$ROOT/src/render_plan.cpp"
  "$ROOT/src/replay_engine.cpp"
  "$ROOT/src/section_table.cpp"
  "$ROOT/src/status.cpp"
  "$ROOT/src/string_pool.cpp"
  "$ROOT/src/style_table.cpp"
  "$ROOT/src/util.cpp"
  "$ROOT/src/validator.cpp"
)

BASE_FLAGS=(
  -std=c++17
  -I"$ROOT/include"
)

mkdir -p "$OUT_DIR"

"$CXX_BIN" ${CXXFLAGS:-} "${BASE_FLAGS[@]}" \
  "${COMMON_SRC[@]}" "$ROOT/fuzz/spool_document_fuzzer.cpp" \
  $FUZZING_ENGINE -o "$OUT_DIR/spool_document_fuzzer"

"$CXX_BIN" ${CXXFLAGS:-} "${BASE_FLAGS[@]}" \
  "${COMMON_SRC[@]}" "$ROOT/fuzz/spool_query_fuzzer.cpp" \
  $FUZZING_ENGINE -o "$OUT_DIR/spool_query_fuzzer"

