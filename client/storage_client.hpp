// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#ifndef _STORAGE_CLIENT_H_
#define _STORAGE_CLIENT_H_

#include "network/fasttransport.h"
#include "network/configuration.h"


class StorageClient : public TransportReceiver
{
  public:
    StorageClient(const transport::Configuration &config,
             Transport *transport,
             uint64_t clientid = 0);
    virtual ~StorageClient();

    // All RPCs this client can invoke
    virtual void InvokeGetNodeId(uint8_t core_id, int serverIdx, const string &request);

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
};

#endif  /* _STORAGE_CLIENT_H_ */
