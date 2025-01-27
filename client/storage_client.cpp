// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include "debug/assert.hpp"
#include "debug/message.hpp"
#include "client/storage_client.hpp"
#include "common/messages.hpp"

//#include <sys/time.h>
//#include <math.h>

#include <random>
using namespace std;

StorageClient::StorageClient(const network::Configuration &config,
                   network::Transport *transport,
                   uint64_t clientid)
    : config(config),
      transport(transport) {

    this->clientid = clientid;
    // Randomly generate a client ID
    // This is surely not the fastest way to get a random 64-bit int,
    // but it should be fine for this purpose.
    while (this->clientid == 0) {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        this->clientid = dis(gen);
        Debug("Client ID: %lu", this->clientid);
    }
    this->lastReqId = 0;

    transport->Register(this, -1);
}

StorageClient::~StorageClient() { }

nodeid_t StorageClient::GetNodeId(uint8_t coreIdx,
                       uint32_t serverIdx,
                       const string &request) {
    uint64_t reqId = ++lastReqId;

    // TODO: find a way to get sending errors (the eRPC's enqueue_request
    // function does not return errors)
    // TODO: deal with timeouts?
    auto *reqBuf = reinterpret_cast<nodeid_request_t *>(
      transport->GetRequestBuf(
        sizeof(nodeid_request_t),
        sizeof(nodeid_response_t)
      )
    );

    reqBuf->req_nr = reqId;
    blocked = true;
    transport->SendRequestToServer(this,
                                    getNodeIdReqType,
                                    serverIdx, coreIdx,
                                    sizeof(nodeid_request_t));
    this->nodeIdReply.serverIdx = serverIdx;
    this->nodeIdReply.coreIdx = coreIdx;
    return this->nodeIdReply;
}

bool StorageClient::EvictNode(uint8_t coreIdx, 
                                    uint32_t serverIdx,
                                    uint64_t node_id,
                                    const string &request) {
    
    uint64_t reqId = ++lastReqId;
    auto *reqBuf = reinterpret_cast<evictnode_request_t *>(
      transport->GetRequestBuf(
        sizeof(evictnode_request_t),
        sizeof(evictnode_response_t)
      )
    );
    reqBuf->req_nr = reqId;
    reqBuf->node_id = node_id;
    memcpy(reqBuf->buffer, request.c_str(), request.size());
    blocked = true;
    transport->SendRequestToServer(this,
                                    evictNodeReqType,
                                    serverIdx, coreIdx,
                                    sizeof(evictnode_request_t));
    return this->evictNodeReply;
}


void StorageClient::ReceiveResponse(uint8_t reqType, char *respBuf) {
    Debug("[%lu] received response", clientid);
    switch(reqType){
        case getNodeIdReqType:
            HandleGetNodeIdReply(respBuf);
            break;
        case evictNodeReqType:
            HandleEvictNodeReply(respBuf);
            break;
        default:
            Warning("Unrecognized request type: %d\n", reqType);
    }
}

void StorageClient::HandleGetNodeIdReply(char *respBuf) {
    auto *resp = reinterpret_cast<nodeid_response_t *>(respBuf);

    Debug(
        "Client received NodeIdReplyMessage for "
        "request %lu.", resp->req_nr);

    if (resp->req_nr != lastReqId) {
        Warning(
            "Client was not expecting a NodeIdReplyMessage for request %lu, "
            "so it is ignoring the request.",
            resp->req_nr);
        return;
    }
    this->nodeIdReply.nodeIdx = resp->id;
    blocked = false;
}


void StorageClient::HandleEvictNodeReply(char *respBuf) {
    auto *resp = reinterpret_cast<evictnode_response_t *>(respBuf);

    Debug(
        "Client received EvictNodeReplyMessage for "
        "request %lu.", resp->req_nr);

    if (resp->req_nr != lastReqId) {
        Warning(
            "Client was not expecting a EvictNodeReplyMessage for request %lu, "
            "so it is ignoring the request.",
            resp->req_nr);
        return;
    }
    this->evictNodeReply = resp->success;
    blocked = false;
}
