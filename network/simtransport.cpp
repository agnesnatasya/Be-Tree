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
#include <cstring>

#include "debug/assert.hpp"
#include "network/simtransport.hpp"

#include <boost/fiber/all.hpp>

namespace network {

SimTransport::SimTransport(
    const network::Configuration &config,
    uint8_t id
) 
: config(config), id(id)
{
    c = new SimAppContext();
}

SimTransport::~SimTransport()
{
}

void SimTransport::Register(TransportReceiver *receiver, int receiverIdx)
{
    assert(receiverIdx < config.n);

    c->server.receiver = receiver;
    this->receiverIdx = receiverIdx;
}

int SimTransport::MAX_DATA_PER_PKT = 16384;

// Used when the client wants to create a request
char *SimTransport::GetRequestBuf(size_t reqLen, size_t respLen)
{
    // create a new request tag
    if (reqLen == 0)
        reqLen = SimTransport::MAX_DATA_PER_PKT;
    if (respLen == 0)
        respLen = SimTransport::MAX_DATA_PER_PKT;
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
    while(!stop) {}
}

void SimTransport::Stop() {
    Debug("Stopping transport!");
    stop = true;
}

bool SimTransport::SendRequestToServer(TransportReceiver *src, uint8_t reqType, uint32_t serverIdx, uint8_t dstRpcIdx, size_t msgLen) {
    c->client.crt_req_tag->src = src;
    c->client.crt_req_tag->reqType = reqType;
    c->server.receiver->ReceiveRequest(
                c->client.crt_req_tag->reqType, 
                c->client.crt_req_tag->req_msgbuf, 
                c->client.crt_req_tag->resp_msgbuf
            );
    while (src->Blocked()) {
        boost::this_fiber::yield();
    }
    c->client.crt_req_tag = nullptr;
    return true;
}

bool SimTransport::SendRequestToAllServers(TransportReceiver *src, uint8_t reqType, uint8_t dstRpcIdx, size_t msgLen) {
    return true;
}

bool SimTransport::SendResponse(uint64_t reqHandleIdx, size_t msgLen) {
    Debug("Sent response, msgLen = %lu\n", msgLen);
    sim_req_tag_t *tag = c->client.crt_req_tag;
    tag->src->ReceiveResponse(tag->reqType, tag->resp_msgbuf);
    return true;
}

bool SimTransport::SendResponse(size_t msgLen) {
    Debug("Sent response, msgLen = %lu\n", msgLen);
    sim_req_tag_t *tag = c->client.crt_req_tag;
    tag->src->ReceiveResponse(tag->reqType, tag->resp_msgbuf);
    return true;
}
}
