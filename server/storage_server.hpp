// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 *   The interface of the store server hosting the nodes of the B3-tree
 *
 **********************************************************************/

#ifndef _STORAGE_SERVER_H_
#define _STORAGE_SERVER_H_

#include "network/configuration.hpp"
#include "network/transport.hpp"

class StorageServerApp
{
  public:
    StorageServerApp();
    virtual ~StorageServerApp() { };

    uint32_t GetNodeId();

  private:
    uint32_t current_id;
};

class StorageServer : network::TransportReceiver
{
  public:
    StorageServer(network::Configuration config, int myIdx,
              network::Transport *transport,
              StorageServerApp *storageApp);
    ~StorageServer();

    // Message handlers.
    void ReceiveRequest(uint8_t reqType, char *reqBuf, char *respBuf) override;
    void ReceiveResponse(uint8_t reqType, char *respBuf) override {}; // TODO: for now, servers
                                                                      // do not need to communicate
                                                                      // with eachother
    bool Blocked() override { return false; };
    // new handlers
    void HandleGetNodeId(char *reqBuf, char *respBuf, size_t &respLen);
    void HandleEvictNode(char *reqBuf, char *respBuf, size_t &respLen);

  private:
    network::Configuration config;
    int myIdx; // Server index into config.
    network::Transport *transport;

    StorageServerApp * storageApp;
};

#endif /* _STORAGE_SERVER_H_ */
