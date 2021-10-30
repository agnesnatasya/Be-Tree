// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 *   Store server hosting the nodes of the B3-tree
 *
 **********************************************************************/
#include "storage_server.hpp"
#include "common/messages.hpp"

using namespace std;

StorageServerApp::StorageServerApp() : current_id(0) {
    current_id++;
}

uint32_t StorageServerApp::GetNodeId() {
   return current_id;
}

StorageServer::StorageServer(network::Configuration config, int myIdx,
                     network::Transport *transport,
                     StorageServerApp *storageApp)
    : config(std::move(config)), myIdx(myIdx), transport(transport),
      storageApp(storageApp)
{
    if (transport != NULL) {
        transport->Register(this, myIdx);
    } else {
        // we use this for micorbenchmarking, but still issue a warning
        Warning("NULL transport provided to the replication layer");
    }
}

StorageServer::~StorageServer() { }

void StorageServer::ReceiveRequest(uint8_t reqType, char *reqBuf, char *respBuf) {
    size_t respLen;
    switch(reqType) {
        case getNodeIdRequestType:
            HandleGetNodeId(reqBuf, respBuf, respLen);
            break;
        default:
            Warning("Unrecognized request type: %d", reqType);
    }

    // For every request, we need to send a response (because we use eRPC)
    if (!(transport->SendResponse(respLen)))
        Warning("Failed to send reply message");
}

void StorageServer::HandleGetNodeId(char *reqBuf, char *respBuf, size_t &respLen) {
    auto *req = reinterpret_cast<nodeid_request_t *>(reqBuf);
    auto *resp = reinterpret_cast<nodeid_response_t *>(respBuf);
    resp->req_nr = req->req_nr;
    resp->id = storageApp->GetNodeId();
    respLen = sizeof(nodeid_response_t);
}
