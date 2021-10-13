// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// https://www.cprogramming.com/tutorial/cpreprocessor.html
#ifndef _NETWORK_GRPCTRANSPORT_H_
#define _NETWORK_GRPCTRANSPORT_H_

#include "network/configuration.hpp"
#include "network/transport.hpp"

#include "util/numautils.h"
#include <gflags/gflags.h>

#include <event2/event.h>

#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <mutex>
#include <atomic>
#include <netinet/in.h>



/**
 * Header RPC stub for gRPC protocol
 */
namespace network
{

    class GrpcTransport : public Transport
    {
    public:
        GrpcTransport(const network::Configuration &config,
                        std::string &ip,
                        int nthreads,
                        uint8_t nr_req_types,
                        uint8_t phy_port,
                        uint8_t numa_node,
                        uint8_t id);
        virtual ~GrpcTransport();
        void Register(TransportReceiver *receiver,
                        int receiverIdx) override;
        void Run();
        void Wait();
        void Stop();
        //int Timer(uint64_t ms, timer_callback_t cb) override;
        //bool CancelTimer(int id) override;
        //void CancelAllTimers() override;

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

        // Number of server threads
        int nthreads;

        // used as the RPC id, must be unique per transport thread
        uint8_t id;

        // Index of the receiver (if -1 then the receiver is a client that
        // does not get requests, otherwise it is a server from the configuration)
        int receiverIdx;
    };

} // namespace network

#endif // _NETWORK_GRPCTRANSPORT_H_
