// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#ifndef _NETWORK_TRANSPORT_H_
#define _NETWORK_TRANSPORT_H_

#include "debug/message.hpp"
#include "network/configuration.hpp"
#include <functional>



#define CLIENT_NETWORK_DELAY 0
#define REPLICA_NETWORK_DELAY 0
#define READ_AT_LEADER 1

namespace network {

class TransportReceiver
{
public:
    virtual ~TransportReceiver();
    virtual void ReceiveRequest(uint8_t reqType, char *reqBuf, char *respBuf) {
        PPanic("Not implemented; did you forget to set the MULTIPLE_ACTIVE_REQUESTS flag accordingly?");
    };
    virtual void ReceiveRequest(uint64_t reqHandleIdx, uint8_t reqType, char *reqBuf, char *respBuf) {
        PPanic("Not implemented; did you forget to set the MULTIPLE_ACTIVE_REQUESTS flag accordingly?");
    };
    virtual void ReceiveResponse(uint8_t reqType, char *respBuf) = 0;
    virtual bool Blocked() = 0;
};

typedef std::function<void (void)> timer_callback_t;

class Transport
{
public:
    virtual ~Transport() {}
    virtual void Register(TransportReceiver *receiver,
                          int receiverIdx) = 0;
    virtual bool SendResponse(size_t msgLen) = 0;
    virtual bool SendResponse(uint64_t bufIdx, size_t msgLen) = 0;
    virtual bool SendRequestToServer(TransportReceiver *src, uint8_t reqType, uint32_t serverIdx, uint8_t coreIdx, size_t msgLen) = 0;
    virtual bool SendRequestToAllServers(TransportReceiver *src, uint8_t reqType, uint8_t coreIdx, size_t msgLen) = 0;
    //virtual int Timer(uint64_t ms, timer_callback_t cb) = 0;
    //virtual bool CancelTimer(int id) = 0;
    //virtual void CancelAllTimers() = 0;

    // Use this to achieve zero copy
    // You do 
    virtual char *GetRequestBuf(size_t reqLen, size_t respLen) = 0;
    virtual int GetSession(TransportReceiver *src, uint8_t receiverIdx, uint8_t dstRpcIdx) = 0;

    virtual uint8_t GetID() = 0;
};

// For when we want to add timeouts to the RPC requests
//class Timeout
//{
//public:
//    Timeout(Transport *transport, uint64_t ms, timer_callback_t cb);
//    virtual ~Timeout();
//    virtual void SetTimeout(uint64_t ms);
//    virtual uint64_t Start();
//    virtual uint64_t Reset();
//    virtual void Stop();
//    virtual bool Active() const;
//    
//private:
//    Transport *transport;
//    uint64_t ms;
//    timer_callback_t cb;
//    int timerId;
//};

} // namespace network

#endif  // _NETWORK_TRANSPORT_H_
