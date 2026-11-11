// SCALE decoder (bcos-codec ScaleDecoderStream), non-vector types: first byte selects the target
// type. The std::vector types (unchecked count in operator>>(std::vector<T>&)) live in
// fuzz_scale_decode_vec so that bug does not starve these decoders.
#include <bcos-codec/scale/Scale.h>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

using namespace bcos;
using namespace bcos::codec::scale;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    const uint8_t mode = data[0] % 8;
    gsl::span<const uint8_t> payload(data + 1, size - 1);
    try
    {
        switch (mode)
        {
        case 0:
        {
            std::string s;
            decode(s, payload);
            break;
        }
        case 1:
        {
            CompactInteger c;
            decode(c, payload);
            break;
        }
        case 2:
        {
            boost::optional<bool> o;
            decode(o, payload);
            break;
        }
        case 3:
        {
            boost::variant<uint8_t, uint32_t, std::string> v;
            decode(v, payload);
            break;
        }
        case 4:
        {
            std::tuple<uint32_t, bool, std::string, int16_t> t;
            decode(t, payload);
            break;
        }
        case 5:
        {
            std::map<uint16_t, std::string> m;
            decode(m, payload);
            break;
        }
        case 6:
        {
            std::array<uint32_t, 4> a;
            decode(a, payload);
            break;
        }
        default:
        {
            u256 u;
            decode(u, payload);
            break;
        }
        }
    }
    catch (...)
    {
    }
    return 0;
}
