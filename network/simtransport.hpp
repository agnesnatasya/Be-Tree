// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * simtransport.h:
 *   simulated message-passing interface for testing use
 *
 * Reference: 2013-2016 Dan R. K. Ports  <drkp@cs.washington.edu>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************************/

#ifndef _NETWORK_SIMTRANSPORT_H_
#define _NETWORK_SIMTRANSPORT_H_

#include "network/configuration.hpp"
#include "network/transport.hpp"

#include <map>

namespace network
{

    class SimulatedTransport : public Transport
    {
    public:
        SimulatedTransport(
            const network::Configuration &config,
            int nthreads,
            uint8_t nr_req_types,
            uint8_t id);
        ~SimulatedTransport();
        void Register(TransportReceiver *receiver, int replicaIdx);
        void Run();
        void Wait();
        void Stop();

        bool SendRequestToServer(TransportReceiver *src, uint8_t reqType, uint32_t serverIdx, uint8_t dstRpcIdx, size_t msgLen) override;
        bool SendRequestToAllServers(TransportReceiver *src, uint8_t reqType, uint8_t dstRpcIdx, size_t msgLen) override;
        bool SendResponse(uint64_t reqHandleIdx, size_t msgLen) override;
        bool SendResponse(size_t msgLen) override;
        char *GetRequestBuf(size_t reqLen, size_t respLen) override;
        int GetSession(TransportReceiver *src, uint8_t replicaIdx, uint8_t dstRpcIdx) override;

        uint8_t GetID() override { return id; };

    protected:
        bool SendMessageInternal(TransportReceiver *src,
                                 const Message &m,
                                 bool multicast);

    private:
        // Configuration containing the ids of the servers
        network::Configuration config;

        // Number of server threads
        int nthreads;

        // This corresponds to the thread id of the curren server
        // used as the RPC id, must be unique per transport thread
        uint8_t id;

        // This corresponds to the machine index
        // Index of the receiver (if -1 then the receiver is a client that
        // does not get requests, otherwise it is a server from the configuration)
        int receiverIdx;

        std::map<int, TransportReceiver *> receivers;
    };
}
#endif  // _NETWORK_SIMTRANSPORT_H_
