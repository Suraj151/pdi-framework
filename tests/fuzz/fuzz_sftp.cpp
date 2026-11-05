/******************************* Fuzz SFTP ************************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The sftp framing: bytes arrive split across ssh channel data, get accumulated,
and are cut back into requests on a length the peer supplies; a write's data is
streamed to its file, and requests held back are taken again on a later turn.

Author          : Suraj I.
created Date    : 26th Aug 2026
******************************************************************************/

#include <FuzzCommon.h>

#if defined(ENABLE_SSH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

#include <MountedStack.h>
#include <service_provider/shell/ssh/SSHServiceprovider.h>

using namespace LWSSH;

namespace
{

    /**
     * @brief Reaches the framing entry points, which the server keeps internal.
     */
    class SftpFuzzServer : public SSHServer
    {

    public:
        /**
         * @brief Feed one chunk in as channel data, then optionally the empty turn
         *        that takes held requests again.
         */
        void feed(LWSSHSession *session, pdiutil::vector<uint8_t> &chunk, bool resume)
        {
            m_session = session;

            handleChannelSubsystemRequest(chunk);
            if (resume && session->m_state != LWSSHSession::SESSION_STATE_SESSION_CLOSE)
            {
                pdiutil::vector<uint8_t> none;
                handleChannelSubsystemRequest(none);
            }

            m_session = nullptr;
        }
    };

    /**
     * @brief The mounted filesystem the sftp requests act on.
     */
    void readyStorage()
    {
        static bool done = false;
        if (!done)
        {
            done = true;
            pdifuzz::useFreeClock();
            pditest::mountedVfs();
        }
    }

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    readyStorage();

    pdifuzz::FuzzInput input(data, size);
    bool resume = 0 != input.pick(2);
    uint8_t chunks = 1 + input.pick(4);

    // the session takes ownership of the client and closes it on the way out,
    // so it cannot be handed one that lives on the stack
    pdifuzz::FuzzClient *client = pdiutil::safe_new<pdifuzz::FuzzClient>(nullptr, (size_t)0);
    if (nullptr == client)
    {
        return 0;
    }

    LWSSHSession session(client);
    session.current_channel.subsystem_req.subsystem = "sftp";

    SftpFuzzServer server;

    const uint8_t *cursor = input.rest();
    size_t left = input.remaining();
    size_t each = (0 == left) ? 0 : ((left + chunks - 1) / chunks);

    while (left > 0 && session.m_state != LWSSHSession::SESSION_STATE_SESSION_CLOSE)
    {
        size_t take = each < left ? each : left;
        pdiutil::vector<uint8_t> chunk(cursor, cursor + take);
        server.feed(&session, chunk, resume);
        cursor += take;
        left -= take;
    }

    return 0;
}

#else

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    return 0;
}

#endif
