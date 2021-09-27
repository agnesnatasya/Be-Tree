// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 *   The interface of the store server hosting the nodes of the B3-tree
 *
 **********************************************************************/

#ifndef _STORAGE_SERVER_H_
#define _STORAGE_SERVER_H_

#include "network/configuration.hpp"
#include "network/fasttransport.hpp"


class StorageServer : TransportReceiver
{
  public:
    StorageServer(transport::Configuration config, int myIdx,
              Transport *transport);
    ~StorageServer();

    // Message handlers.
    void ReceiveRequest(uint8_t reqType, char *reqBuf, char *respBuf) override;
    void ReceiveResponse(uint8_t reqType, char *respBuf) override {}; // TODO: for now, servers
                                                                      // do not need to communicate
                                                                      // with eachother
    bool Blocked() override { return false; };
    // new handlers
    void HandleGetNodeId(char *reqBuf, char *respBuf, size_t &respLen);

  private:
    transport::Configuration config;
    int myIdx; // Server index into config.
    Transport *transport;

};

#endif /* _STORAGE_SERVER_H_ */
