// Known-answer probe CLI for mayhem/test.sh: prints hex for a few well-known vectors.
#include <bcos-codec/abi/ContractABICodec.h>
#include <bcos-codec/scale/Scale.h>
#include <bcos-crypto/hash/Keccak256.h>
#include <bcos-crypto/hash/Sha256.h>
#include <bcos-crypto/hash/Sha3.h>
#include <bcos-utilities/Base64.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <iostream>
#include <string>

using namespace bcos;

int main(int argc, char** argv)
{
    if (argc < 2)
        return 2;
    const std::string op = argv[1];
    const std::string arg = argc > 2 ? argv[2] : "";
    static const crypto::Keccak256 hashImpl;
    if (op == "sha256" || op == "sha3")
    {
        bytesConstRef in((const byte*)arg.data(), arg.size());
        std::cout << (op == "sha256" ? crypto::sha256Hash(in) : crypto::sha3Hash(in)).hex() << "\n";
        return 0;
    }
    if (op == "base64")
    {
        std::cout << base64Encode(arg) << "\n";
        return 0;
    }
    if (op == "abi-baz")
    {
        codec::abi::ContractABICodec codec(hashImpl);
        std::cout << codec.abiInHex("", u256(69), true) << "\n";
        return 0;
    }
    if (op == "abi-string-roundtrip")
    {
        codec::abi::ContractABICodec codec(hashImpl);
        auto enc = codec.abiIn("", arg);
        std::string out;
        codec.abiOut(ref(enc), out);
        std::cout << toHex(enc) << " " << out << "\n";
        return 0;
    }
    if (op == "scale-compact")
    {
        codec::scale::CompactInteger v(std::stoull(arg));
        std::cout << toHex(codec::scale::encode(v)) << "\n";
        return 0;
    }
    return 2;
}
