// Solidity ABI type-string and function-signature parsers (bcos-codec ContractABIType).
#include <bcos-codec/abi/ContractABIType.h>
#include <cstddef>
#include <cstdint>
#include <string>

using namespace bcos::codec::abi;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string text(reinterpret_cast<const char*>(data), size);

    ABIInType type;
    if (type.reset(text))
    {
        (void)type.valid();
        (void)type.dynamic();
        (void)type.getType();
        (void)type.getEleType();
        auto rank = type.rank();
        for (std::size_t i = 0; i < rank; ++i)
            (void)type.extent(i);
        while (type.removeExtent())
        {
        }
    }

    ABIFunc func;
    if (func.parser(text))
    {
        (void)func.getSignature();
        (void)func.getFuncName();
        (void)func.getParamsType();
    }
    return 0;
}
