// Solidity ABI decoder (bcos-codec ContractABICodec), non-vector shapes: first byte selects the
// expected parameter shape, the rest is the calldata/returndata to decode. The std::vector shapes
// (unchecked `resize(length)` in deserialize(std::vector<T>&)) live in fuzz_abi_decode_vec so that
// bug does not starve these decoders.
#include <bcos-codec/abi/ContractABICodec.h>
#include <bcos-crypto/hash/Keccak256.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

using namespace bcos;
using namespace bcos::codec::abi;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    static const crypto::Keccak256 hashImpl;
    ContractABICodec codec(hashImpl);
    const uint8_t mode = data[0] % 7;
    bytesConstRef payload(data + 1, size - 1);

    switch (mode)
    {
    case 0:
    {
        std::string s;
        codec.abiOut(payload, s);
        break;
    }
    case 1:
    {
        bytes b;
        codec.abiOut(payload, b);
        break;
    }
    case 2:
    {
        u256 u;
        s256 i;
        codec.abiOut(payload, u, i);
        break;
    }
    case 3:
    {
        bool flag;
        Address addr;
        string32 word;
        codec.abiOut(payload, flag, addr, word);
        break;
    }
    case 4:
    {
        std::tuple<u256, std::string, Address> t;
        codec.abiOut(payload, t);
        break;
    }
    case 5:
    {
        std::array<u256, 3> a;
        bytes b;
        codec.abiOut(payload, a, b);
        break;
    }
    default:
    {
        // abiOut() reports bad input by returning false; this entry point throws instead.
        std::vector<std::string> out;
        try
        {
            codec.abiOutByFuncSelector(payload, {"int", "uint256", "address", "string"}, out);
        }
        catch (const std::exception&)
        {
        }
        break;
    }
    }
    return 0;
}
