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
#include "network/app_mem_pool.cpp"

#include <map>
#include <deque>

#define MULTIPLE_ACTIVE_REQUESTS false

namespace network {

// A tag attached to every request we send;
// it is passed to the response function
struct sim_req_tag_t
{
    char *req_msgbuf;
    char *resp_msgbuf;
    uint8_t reqType;
    TransportReceiver *src;
};

class SimRpc {
    public:
        void free_msg_buffer(sim_req_tag_t* req_tag)
        {
            delete[] req_tag->req_msgbuf;
            delete[] req_tag->resp_msgbuf;
        }

        void resize_msg_buffer(char* msgBuf, int newMsgLen) {
            msgBuf = new char[newMsgLen];
        }

        char* alloc_msg_buffer(int msgLen) {
            return new char[msgLen];
        }

        // void enqueue_request(sim_req_tag_t* req_tag)
        // {
        //     req_queue.push_back(req_tag);
        // }

        // std::deque<sim_req_tag_t*> req_queue;
};

class SimAppContext
{
    public:
        struct
        {
            // This is maintained between calls to GetReqBuf and SendRequest
            // to reduce copying
            sim_req_tag_t *crt_req_tag;

            // Request tags used for RPCs exchanged with the servers
            AppMemPool<sim_req_tag_t> req_tag_pool;

        } client;

        struct
        {
            // current req_handle
            TransportReceiver *receiver = nullptr;
        } server;

        // common to both servers and clients
        SimRpc *rpc = new SimRpc();
};

class SimTransport : public Transport
{
    public:
        static int MAX_DATA_PER_PKT;
        SimTransport(
            const network::Configuration &config,
            uint8_t id);
        virtual ~SimTransport();
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

    private:
        // Configuration containing the ids of the servers
        network::Configuration config;

        SimAppContext *c;

        // Number of server threads
        int nthreads;

        // This corresponds to the thread id of the current server
        // used as the RPC id, must be unique per transport thread
        uint8_t id;

        // This corresponds to the machine index
        // Index of the receiver (if -1 then the receiver is a client that
        // does not get requests, otherwise it is a server from the configuration)
        int receiverIdx;

        bool stop = false;
    };
}
#endif  // _NETWORK_SIMTRANSPORT_H_
