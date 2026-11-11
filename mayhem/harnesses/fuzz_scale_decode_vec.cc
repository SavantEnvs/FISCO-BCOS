// SCALE decoder (bcos-codec ScaleDecoderStream), std::vector types: first byte selects the target
// type. Split from fuzz_scale_decode because vector decode trusts the declared item count
// (ScaleDecoderStream.h:239, reported once per element type). Only two representative element
// types are instantiated, so that one root cause is reported twice rather than once per type.
#include <bcos-codec/scale/Scale.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace bcos;
using namespace bcos::codec::scale;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    const uint8_t mode = data[0] % 2;
    gsl::span<const uint8_t> payload(data + 1, size - 1);
    try
    {
        if (mode == 0)
        {
            std::vector<uint8_t> v;
            decode(v, payload);
        }
        else
        {
            std::vector<std::string> v;
            decode(v, payload);
        }
    }
    catch (...)
    {
    }
    return 0;
}
