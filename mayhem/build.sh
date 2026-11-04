#!/usr/bin/env bash
# Builds halley's engine core (+ bundled contrib) twice: sanitized for the fuzz harnesses, normal for gtest.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"
OUT=/mayhem
B="$SRC/mayhem/build"
rm -rf "$B"
mkdir -p "$B/fuzz" "$B/test"

CORE=src/engine/core
CONTRIB=src/contrib
INC=(-Iinclude -I"$CORE/include" -I"$CORE/include/halley/core" -Ishared_gen/cpp -I"$CONTRIB" -I"$CONTRIB/yaml-cpp/include" -I"$CONTRIB/lua/src")
DEFS=(-DDEV_BUILD)

CONTRIB_C=(zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c zlib/zutil.c lz4/lz4.c lz4/lz4file.c lz4/lz4frame.c lz4/lz4hc.c miniz/miniz.c stb_image/stb_image.c qoi/qoi.c xxhash/xxhash.c tiny-aes/aes.c)
CONTRIB_CXX=(json/jsoncpp.cpp lodepng/lodepng.cpp)
for f in "$CONTRIB"/yaml-cpp/src/*.cpp; do CONTRIB_CXX+=("${f#"$CONTRIB"/}"); done

CORE_SRCS=()
for d in bytes data_structures file file_formats maths memory storage support text utils concurrency time resources; do
  for f in "$CORE/src/$d"/*.cpp; do CORE_SRCS+=("$f"); done
done
CORE_SRCS+=("$CORE/src/os/os.cpp" "$CORE/src/os/os_linux.cpp" "$CORE/src/os/os_unix.cpp")
CORE_SRCS+=("$CORE/src/support/StackWalker"/*.cpp mayhem/entity_stubs.cpp)

# compile_all <objdir> <flags...>: compile every source in parallel into <objdir>, archive libhalley.a
compile_all() {
  local objdir="$1"; shift
  local flags="$*"
  local cflags="-w -msse4.2 $flags"
  local cxxflags="-std=c++20 -w -msse4.2 -include cstdint $flags ${INC[*]} ${DEFS[*]}"
  mkdir -p "$objdir"
  {
    for f in "${CONTRIB_C[@]}"; do echo "$CC $cflags -c $CONTRIB/$f -o $objdir/c_${f//\//_}.o"; done
    for f in "${CONTRIB_CXX[@]}"; do echo "$CXX $cxxflags -c $CONTRIB/$f -o $objdir/x_${f//\//_}.o"; done
    for f in "${CORE_SRCS[@]}"; do echo "$CXX $cxxflags -c $f -o $objdir/x_${f//\//_}.o"; done
  } | xargs -P "$MAYHEM_JOBS" -d '\n' -I{} bash -c '{}'
  rm -f "$objdir/libhalley.a"
  ar rcs "$objdir/libhalley.a" "$objdir"/*.o
}

# Halley's Vector derefs data() of empty vectors and memcpy()s zero bytes to null; lz4 does null+0. These
# benign UBSan hits (null, nonnull-attribute, pointer-overflow) would halt every input, so only they are relaxed.
RELAX_UBSAN="-fno-sanitize=null,nonnull-attribute,pointer-overflow"

# 1) sanitized, instrumented engine for the fuzz harnesses
compile_all "$B/fuzz" -O1 $SANITIZER_FLAGS $RELAX_UBSAN $DEBUG_FLAGS -fsanitize=fuzzer-no-link

# 2) normal-flags engine + gtest suite
compile_all "$B/test" -O1 $COVERAGE_FLAGS
TESTS=()
# vector_test.cpp uses `T::value_type()` without `typename`, which clang rejects
for f in src/tests/src/*.cpp; do [ "$(basename "$f")" = vector_test.cpp ] || TESTS+=("$f"); done
TESTS+=(mayhem/kat_test.cpp)
$CXX -std=c++20 -w -msse4.2 -include cstdint -O1 $COVERAGE_FLAGS "${INC[@]}" "${DEFS[@]}" "${TESTS[@]}" "$B/test/libhalley.a" \
  -lgtest -lgtest_main -lpthread -ldl -o "$OUT/halley-tests" $COVERAGE_FLAGS

# 3) harnesses: libFuzzer + standalone, LSan off
FUZZ_FLAGS="$SANITIZER_FLAGS $RELAX_UBSAN $DEBUG_FLAGS"
$CXX -std=c++20 -O1 $FUZZ_FLAGS -c mayhem/lsan_off.cc -o "$B/lsan_off.o"
$CC $FUZZ_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
for h in mayhem/harnesses/fuzz_*.cpp; do
  n=$(basename "$h" .cpp)
  $CXX -std=c++20 -w -msse4.2 -O1 $FUZZ_FLAGS -fsanitize=fuzzer-no-link "${INC[@]}" "${DEFS[@]}" -c "$h" -o "$B/$n.o"
  $CXX $FUZZ_FLAGS $LIB_FUZZING_ENGINE "$B/$n.o" "$B/lsan_off.o" "$B/fuzz/libhalley.a" -lpthread -ldl -o "$OUT/$n"
  $CXX $FUZZ_FLAGS "$B/$n.o" "$B/standalone_main.o" "$B/lsan_off.o" "$B/fuzz/libhalley.a" -lpthread -ldl -o "$OUT/$n-standalone"
done
