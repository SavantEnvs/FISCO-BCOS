// WebSocket SDK/RPC message framing decoder (bcos-boostssl WsMessage).
#include <bcos-boostssl/websocket/WsMessage.h>
#include <boost/log/core.hpp>
#include <cstddef>
#include <cstdint>

using namespace bcos;
using namespace bcos::boostssl::ws;

// Decoder errors are logged to stdout through boost::log; silence them for throughput.
extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    boost::log::core::get()->set_logging_enabled(false);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        WsMessage msg;
        if (msg.decode(bytesConstRef(data, size)) > 0)
        {
            bytes out;
            msg.encode(out);
            (void)msg.seq();
            (void)msg.payload()->size();
            (void)msg.isRespPacket();
        }
    }
    catch (...)
    {
    }
    return 0;
}
