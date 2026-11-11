#!/usr/bin/env bash
#
# mayhem/test.sh — RUN upstream's bcos-codec unit tests (Boost.Test, built by mayhem/build.sh with
# normal flags) and a set of known-answer probes. Builds nothing.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

OUT="$SRC/mayhem/build-mayhem"
for b in codec-test kat; do
  [ -x "$OUT/$b" ] || { echo "missing $OUT/$b: mayhem/build.sh did not build it" >&2; emit_ctrf "boost.test+kat" 0 1; exit 1; }
done

# 1) The Boost.Test suite: parse its own summary ("N test cases out of M passed"). A neutered
#    binary prints nothing, so the match fails and the whole run is reported as failed.
log=$("$OUT/codec-test" --color_output=no --report_level=short 2>&1)
passed=0; failed=0
if [[ "$log" =~ ([0-9]+)\ test\ cases\ out\ of\ ([0-9]+)\ passed ]]; then
  passed=${BASH_REMATCH[1]}
  failed=$(( BASH_REMATCH[2] - passed ))
elif [[ "$log" =~ ([0-9]+)\ test\ cases\ out\ of\ ([0-9]+)\ failed ]]; then
  failed=${BASH_REMATCH[1]}
  passed=$(( BASH_REMATCH[2] - failed ))
else
  failed=1
  echo "codec-test printed no Boost.Test summary" >&2
fi
if [ "$passed" -lt 60 ]; then
  echo "only $passed codec test cases passed (expected >= 60)" >&2
  failed=$(( failed + 1 ))
fi

# 2) Known-answer probes, each compared with an independently published value.
kat() {  # kat <name> <expected> <op> [arg]
  local name="$1" want="$2" got
  shift 2
  got=$("$OUT/kat" "$@" 2>/dev/null)
  if [ "$got" = "$want" ]; then
    passed=$(( passed + 1 ))
  else
    failed=$(( failed + 1 ))
    echo "KAT $name: want '$want' got '$got'" >&2
  fi
}
kat sha256-abc ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad sha256 abc
kat sha3-abc 3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532 sha3 abc
kat sha3-empty a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a sha3 ""
kat base64-hello aGVsbG8= base64 hello
kat base64-foobar Zm9vYmFy base64 foobar
kat abi-uint-bool 00000000000000000000000000000000000000000000000000000000000000450000000000000000000000000000000000000000000000000000000000000001 abi-baz
kat abi-string-hi "000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000026869000000000000000000000000000000000000000000000000000000000000 hi" abi-string-roundtrip hi
kat scale-compact-69 1501 scale-compact 69
kat scale-compact-1000 a10f scale-compact 1000
kat scale-compact-0 00 scale-compact 0

emit_ctrf "boost.test+kat" "$passed" "$failed"
