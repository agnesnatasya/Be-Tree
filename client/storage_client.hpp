// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#ifndef _STORAGE_CLIENT_H_
#define _STORAGE_CLIENT_H_

#include "network/configuration.hpp"
#include "network/transport.hpp"
// #include "common/messages.hpp"

struct nodeid_t {
    uint32_t serverIdx;
    uint32_t coreIdx;
    uint32_t nodeIdx;
};

class StorageClient : public network::TransportReceiver
{
  public:
    StorageClient(const network::Configuration &config,
             network::Transport *transport,
             uint64_t clientid = 0);
    virtual ~StorageClient();

    // All RPCs this client can invoke
    virtual nodeid_t GetNodeId(uint8_t coreIdx, uint32_t serverIdx, const string &request);
    virtual bool EvictNode(uint8_t coreIdx, uint32_t serverIdx, uint64_t node_id, const string &request);

    // Inherited from TransportReceiver
    void ReceiveRequest(uint8_t reqType, char *reqBuf, char *respBuf) override { PPanic("Not implemented."); };
    void ReceiveResponse(uint8_t reqType, char *respBuf) override;
    bool Blocked() override { return blocked; };

protected:

    network::Configuration config;
    network::Transport *transport;
    uint64_t lastReqId;

    // We assume this client is single-threaded and synchronous

    uint64_t clientid;
    bool blocked;

    // Handlers for replies to the RPC calls
    void HandleGetNodeIdReply(char *respBuf);
    void HandleEvictNodeReply(char *respBuf);
    nodeid_t nodeIdReply;
    bool evictNodeReply;
};

#endif  /* _STORAGE_CLIENT_H_ */
