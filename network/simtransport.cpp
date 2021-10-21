// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * simtransport.cc:
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

#include <iostream>
#include "network/simtransport.hpp"

using namespace network;
{

static void simtransport_response(void *_context, void *_tag)
{
    auto *c = static_cast<SimAppContext *>(_context);
    auto *rt = reinterpret_cast<sim_req_tag_t *>(_tag);
    Debug("Received respose, reqType = %d", rt->reqType);
    rt->src->ReceiveResponse(rt->reqType,
                             reinterpret_cast<char *>(rt->resp_msgbuf.buf));
    c->rpc->free_msg_buffer();
    c->client.req_tag_pool.free(rt);
    }

    // Function called when the server received a request
    // (clients never receive requests, just responses)
    static void simtransport_request(void *_context, void *_tag)
    {
        // save the req_handle for when we are in the SendMessage function
        auto *c = static_cast<SimAppContext *>(_context);
#if MULTIPLE_ACTIVE_REQUESTS
        // upcall to the app
#else
        // upcall to the app
        c->server.receiver->ReceiveRequest(c->cilent.crt_req_tag->reqType,
                                           c->cilent.crt_req_tag->req_msgbuf,
                                           c->cilent.crt_req_tag->resp_msgbuf);
#endif
    }

    SimTransport::SimTransport(
        const network::Configuration &config,
        uint8_t id
    ) : config(config), id(id)
    {
        context = new SimAppContext();
    }

    SimTransport::~SimTransport()
    {
    }

    void SimTransport::Register(TransportReceiver *receiver, int receiverIdx)
    {
        assert(receiverIdx < config.n);
        
        if (receiverIdx > -1)
            c->server.receiver = receiver;
        this->receiverIdx = receiverIdx;
    }

    int SimTransport::MAX_DATA_PER_PKT = 16384;

    // Used when the client wants to create a request
    char SimTransport::*
    GetRequestBuf(size_t reqLen, size_t respLen)
    {
        // create a new request tag
        if (reqLen == 0)
            reqLen = SimTransport.MAX_DATA_PER_PKT;
        if (respLen == 0)
            respLen = SimTransport.MAX_DATA_PER_PKT;
        c->client.crt_req_tag = c->client.req_tag_pool.alloc();
        c->client.crt_req_tag->req_msgbuf = new char[reqLen];
        c->client.crt_req_tag->resp_msgbuf = new char[respLen];
        return c->client.crt_req_tag->req_msgbuf;
    }

    int SimTransport::GetSession(TransportReceiver *src, uint8_t replicaIdx, uint8_t dstRpcIdx)
    {
        return -1;
    }

    void SimTransport::Run()
    {
        // If it's client, just busy waiting
        if (receiverIdx == -1) {
            while(!stop);
        }

        std::cout << "hi\n";
        // If it's server, check the request queue
        while(!stop) {
            std::cout << "h2 \n";
            if (!c.req_queue.empty()) {
                sim_req_tag_t req_tag = c.req_queue.front();
                req_tag.src->ReceiveRequest(req_tag.reqType, req_tag.req_msgbuf, req_tag.req_respbuf);
            }
        }
    }

    void SimTransport::Stop() {
        Debug("Stopping transport!");
        stop = true;
    }

    bool SimTransport::SendRequestToServer(TransportReceiver *src, uint8_t reqType, uint32_t serverIdx, uint8_t dstRpcIdx, size_t msgLen) {
        c->client.crt_req_tag->src = src;
        c->client.crt_req_tag->reqType = reqType;
        c->enqueue_request(c->client.crt_req_tag);
    }

    bool SimTransport::SendRequestToAllServers(TransportReceiver *src, uint8_t reqType, uint8_t dstRpcIdx, size_t msgLen) {
        for (int i = 0; i < config.n; i++)
        {
            // skip the sending entity
            if (this->receiverIdx == i)
                continue;

            if (i == config.n - 1)
            {
                c->client.crt_req_tag->src = src;
                c->client.crt_req_tag->reqType = reqType;
                c->rpc->enqueue_request(c->client.crt_req_tag);
            }
            else
            {
                // need to use different erpc::MsgBuffer per session
                auto *rt = c->client.req_tag_pool.alloc();
                rt->req_msgbuf = new char[reqLen];
                rt->resp_msgbuf = new char[respLen];
                rt->reqType = reqType;
                rt->src = src;
                std::memcpy(rt->req_msgbuf, c->client.crt_req_tag->req_msgbuf, msgLen);
                c->rpc->enqueue_request(rt);
            }
        }
        if (this->receiverIdx == config.n - 1)
        {
            // TODO: free the current buffer
        }
    }

    bool SimTransport::SendResponse(uint64_t reqHandleIdx, size_t msgLen) {
        Debug("Sent response, msgLen = %lu\n", msgLen);
        simtransport_response(c, c->client.crt_req_tag);
        return true;
    }

    bool SimTransport::SendResponse(size_t msgLen) {
        Debug("Sent response, msgLen = %lu\n", msgLen);
        simtransport_response(c, c->client.crt_req_tag);
        return true;
    }
}
