#!/usr/bin/env bash
#
# mayhem/build.sh — build the FISCO-BCOS fuzz harnesses and the codec unit-test oracle.
#
# Upstream's build needs vcpkg + the tars toolchain and fetches dozens of dependencies, so this
# compiles only the sources the targets exercise (bcos-utilities, bcos-codec, the P2P/WebSocket
# message codecs and the Keccak/SHA hash wrappers) directly against Debian's -dev packages.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
OUT="$SRC/mayhem/build-mayhem"
FUZZ="$OUT/fuzz"
TEST="$OUT/test"
mkdir -p "$FUZZ" "$TEST"

INCLUDES="-I$SRC/bcos-utilities -I$SRC/bcos-codec -I$SRC/bcos-crypto -I$SRC/bcos-framework -I$SRC/bcos-gateway -I$SRC/bcos-boostssl -I$SRC/bcos-utilities/testutils -I$SRC"
DEFS="-DBOOST_SPIRIT_THREADSAFE -DTBB_SUPPRESS_DEPRECATED_MESSAGES=1 -DBOOST_LOG_DYN_LINK"
LIBS="-lssl -lcrypto -lboost_log -lboost_log_setup -lboost_thread -lboost_filesystem -lboost_chrono -lboost_regex -lboost_system -lboost_iostreams -lz -lzstd -lpthread -lm"

# The project sources the harnesses and the unit tests need.
LIB_SRCS="
bcos-utilities/bcos-utilities/Common.cpp
bcos-utilities/bcos-utilities/BoostLog.cpp
bcos-utilities/bcos-utilities/GzTools.cpp
bcos-utilities/bcos-utilities/DataConvertUtility.cpp
bcos-utilities/bcos-utilities/FixedBytes.cpp
bcos-utilities/bcos-utilities/Exceptions.cpp
bcos-utilities/bcos-utilities/Base64.cpp
bcos-utilities/bcos-utilities/ZstdCompress.cpp
bcos-crypto/bcos-crypto/hash/HashImpl.cpp
bcos-codec/bcos-codec/abi/ContractABICodec.cpp
bcos-codec/bcos-codec/abi/ContractABIType.cpp
bcos-codec/bcos-codec/scale/ScaleDecoderStream.cpp
bcos-codec/bcos-codec/scale/ScaleEncoderStream.cpp
bcos-gateway/bcos-gateway/libnetwork/Message.cpp
bcos-gateway/bcos-gateway/libp2p/P2PMessage.cpp
bcos-gateway/bcos-gateway/libp2p/P2PMessageV2.cpp
bcos-boostssl/bcos-boostssl/interfaces/MessageFace.cpp
bcos-boostssl/bcos-boostssl/websocket/WsMessage.cpp
"
TEST_SRCS="
bcos-codec/test/unittests/ContractABICodecOutTest.cpp
bcos-codec/test/unittests/ContractABICodecTest.cpp
bcos-codec/test/unittests/ContractABITypeParseTest.cpp
bcos-codec/test/unittests/RLPTest.cpp
bcos-codec/test/unittests/ScaleCodecTest.cpp
bcos-codec/test/unittests/ScaleDecoderStreamTest.cpp
bcos-codec/test/unittests/main/main.cpp
"
HARNESSES="fuzz_abi_decode fuzz_abi_decode_vec fuzz_abi_type fuzz_scale_decode fuzz_scale_decode_vec fuzz_rlp_decode fuzz_p2p_message fuzz_ws_message"

# 1) Project sources, instrumented: sanitizers + SanCov (fuzzer-no-link) + DWARF 3.
FUZZ_CXXFLAGS="-std=c++20 -O1 $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS $INCLUDES $DEFS"
obj_name() { echo "$1" | tr '/' '_' | sed -E 's/\.(cpp|cc)$/.o/'; }
# The P2P/WS decoders read big-endian fields through unaligned uint16_t*/uint32_t* casts (benign on
# x86), so only those three TUs drop UBSan's alignment check; everything else stays fully checked.
relax_for() { case "$1" in */P2PMessage.cpp|*/P2PMessageV2.cpp|*/WsMessage.cpp) echo "-fno-sanitize=alignment" ;; esac; }
compile_fuzz() { $CXX $FUZZ_CXXFLAGS $(relax_for "$1") -c "$SRC/$1" -o "$FUZZ/$(obj_name "$1")"; }
compile_test() { $CXX -std=c++20 -O1 -g $COVERAGE_FLAGS $INCLUDES $DEFS -c "$SRC/$1" -o "$TEST/$(obj_name "$1")"; }
export -f obj_name relax_for compile_fuzz compile_test
export FUZZ TEST FUZZ_CXXFLAGS INCLUDES DEFS

printf '%s\n' $LIB_SRCS | xargs -P "$MAYHEM_JOBS" -I{} bash -c 'compile_fuzz {}'

# LeakSanitizer off at build time; standalone driver compiled as C.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FUZZ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ/standalone_main.o"

# 2) Each harness twice: libFuzzer binary and run-once standalone reproducer.
LIB_OBJS=$(for s in $LIB_SRCS; do echo "$FUZZ/$(obj_name "$s")"; done)
for h in $HARNESSES; do
  $CXX $FUZZ_CXXFLAGS -c "$SRC/mayhem/harnesses/$h.cc" -o "$FUZZ/$h.o" &
done
wait
for h in $HARNESSES; do
  $CXX $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
      "$FUZZ/$h.o" $LIB_OBJS "$FUZZ/lsan_off.o" $LIBS -o "$SRC/$h" &
  $CXX $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS \
      "$FUZZ/$h.o" "$FUZZ/standalone_main.o" $LIB_OBJS "$FUZZ/lsan_off.o" $LIBS -o "$SRC/$h-standalone" &
done
wait

# 3) Upstream's bcos-codec unit tests, normal flags (no sanitizers, no -gdwarf-3): the test.sh oracle.
printf '%s\n' $LIB_SRCS $TEST_SRCS mayhem/kat.cc | xargs -P "$MAYHEM_JOBS" -I{} bash -c 'compile_test {}'
TEST_OBJS=$(for s in $LIB_SRCS $TEST_SRCS; do echo "$TEST/$(obj_name "$s")"; done)
$CXX $COVERAGE_FLAGS $TEST_OBJS $LIBS -o "$OUT/codec-test"
KAT_OBJS=$(for s in $LIB_SRCS mayhem/kat.cc; do echo "$TEST/$(obj_name "$s")"; done)
$CXX $COVERAGE_FLAGS $KAT_OBJS $LIBS -o "$OUT/kat"
echo "build ok"
