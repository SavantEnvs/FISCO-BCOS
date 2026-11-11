// RLP decoder (bcos-codec rlp): first byte selects the target type.
#include <bcos-codec/rlp/RLPDecode.h>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

using namespace bcos;
using namespace bcos::codec::rlp;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    const uint8_t mode = data[0] % 8;
    bytes buffer(data + 1, data + size);
    bytesRef from(buffer.data(), buffer.size());

    switch (mode)
    {
    case 0:
    {
        (void)decodeHeader(from);
        break;
    }
    case 1:
    {
        bytes out;
        (void)decode(from, out);
        break;
    }
    case 2:
    {
        uint64_t out = 0;
        (void)decode(from, out);
        break;
    }
    case 3:
    {
        bool out = false;
        (void)decode(from, out);
        break;
    }
    case 4:
    {
        std::vector<bytes> out;
        (void)decode(from, out);
        break;
    }
    case 5:
    {
        std::vector<std::vector<uint64_t>> out;
        (void)decode(from, out);
        break;
    }
    case 6:
    {
        uint64_t a = 0;
        bytes b;
        bool c = false;
        (void)decode(from, a, b, c);
        break;
    }
    default:
    {
        u256 out;
        (void)decode(from, out);
        break;
    }
    }
    return 0;
}
