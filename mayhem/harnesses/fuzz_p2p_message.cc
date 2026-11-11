// Gateway P2P wire-message decoder (bcos-gateway P2PMessage / P2PMessageV2):
// first byte selects the message version, the rest is the frame as read off the socket.
#include <bcos-gateway/libp2p/P2PMessage.h>
#include <bcos-gateway/libp2p/P2PMessageV2.h>
#include <boost/log/core.hpp>
#include <cstddef>
#include <cstdint>

using namespace bcos;
using namespace bcos::gateway;

// Decoder errors are logged to stdout through boost::log; silence them for throughput.
extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    boost::log::core::get()->set_logging_enabled(false);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;
    bytesConstRef frame(data + 1, size - 1);
    try
    {
        std::shared_ptr<P2PMessage> msg;
        if (data[0] & 1)
            msg = std::make_shared<P2PMessageV2>();
        else
            msg = std::make_shared<P2PMessage>();

        if (msg->decode(frame) > 0)
        {
            bytes out;
            msg->encode(out);
            (void)msg->payload().size();
            (void)msg->options().dstNodeIDs().size();
            (void)msg->isRespPacket();
        }
    }
    catch (...)
    {
    }
    return 0;
}
