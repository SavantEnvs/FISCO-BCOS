// Solidity ABI decoder (bcos-codec ContractABICodec), std::vector shapes: first byte selects the
// expected parameter shape, the rest is the calldata/returndata to decode. Split from
// fuzz_abi_decode because every vector decode trusts the declared length (ContractABICodec.h:630,
// reported once per element type). Only two representative element types are instantiated, so
// that one root cause is reported twice rather than once per shape.
#include <bcos-codec/abi/ContractABICodec.h>
#include <bcos-crypto/hash/Keccak256.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace bcos;
using namespace bcos::codec::abi;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    static const crypto::Keccak256 hashImpl;
    ContractABICodec codec(hashImpl);
    const uint8_t mode = data[0] % 2;
    bytesConstRef payload(data + 1, size - 1);

    if (mode == 0)
    {
        std::vector<u256> v;
        codec.abiOut(payload, v);
    }
    else
    {
        std::vector<std::string> v;
        codec.abiOut(payload, v);
    }
    return 0;
}
