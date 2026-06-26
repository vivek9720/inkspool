CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -O2 -g
LDFLAGS ?=

COMMON_SRC := \
	src/byte_view.cpp \
	src/checksum.cpp \
	src/diagnostic.cpp \
	src/document.cpp \
	src/format_writer.cpp \
	src/indexer.cpp \
	src/lexer.cpp \
	src/page_script.cpp \
	src/parser.cpp \
	src/query.cpp \
	src/render_plan.cpp \
	src/replay_engine.cpp \
	src/section_table.cpp \
	src/status.cpp \
	src/string_pool.cpp \
	src/style_table.cpp \
	src/util.cpp \
	src/validator.cpp

COMMON_OBJ := $(COMMON_SRC:src/%.cpp=obj/%.o)

.PHONY: all clean test fuzzers

all: bin/inkspool_inspect bin/unit_tests

obj:
	mkdir -p obj

bin:
	mkdir -p bin

obj/%.o: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin/inkspool_inspect: $(COMMON_OBJ) tools/inkspool_inspect.cpp | bin
	$(CXX) $(CXXFLAGS) tools/inkspool_inspect.cpp $(COMMON_OBJ) $(LDFLAGS) -o $@

bin/unit_tests: $(COMMON_OBJ) tests/unit_tests.cpp | bin
	$(CXX) $(CXXFLAGS) tests/unit_tests.cpp $(COMMON_OBJ) $(LDFLAGS) -o $@

test: bin/unit_tests
	./bin/unit_tests

fuzzers:
	@echo "Use .clusterfuzzlite/build.sh with CXXFLAGS and LIB_FUZZING_ENGINE."

clean:
	rm -rf bin obj out build

