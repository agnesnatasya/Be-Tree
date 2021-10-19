// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/**
 * RPC stub for eRPC protocol
 * Handles buffering and async calls
 */

#ifndef _NETWORK_FASTTRANSPORT_H_
#define _NETWORK_FASTTRANSPORT_H_

#include "network/configuration.hpp"
#include "network/transport.hpp"

#include "rpc.h"
#include "rpc_constants.h"

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

#include <boost/unordered_map.hpp>

namespace network {

// This would make fast transport to maintain a map of request handles
// and send response to the indicated one
// (will have a bit more overhead but required for the leader based replication
// schemes)
#define MULTIPLE_ACTIVE_REQUESTS false

/*
 * Class FastTransport implements a multi-threaded
 * transport layer based on eRPC which works with
 * a client - server configuration, where the server
 * may have multiple replicas.
 *
 * The Register function is used to register a transport
 * receiver. The transport is responsible for sending and
 * dispatching messages from/to its receivers accordingly.
 * A transport receiver can either be a client or a server
 * replica. A transport instance's receivers must be
 * of the same type.
 */

// A tag attached to every request we send;
// it is passed to the response function
struct req_tag_t {
    erpc::MsgBuffer req_msgbuf;
    erpc::MsgBuffer resp_msgbuf;
    uint8_t reqType;
    TransportReceiver *src;
};

// A basic mempool for preallocated objects of type T. eRPC has a faster,
// hugepage-backed one.
template <class T> class AppMemPool {
    public:
        size_t num_to_alloc = 1;
        std::vector<T *> backing_ptr_vec;
        std::vector<T *> pool;

    void extend_pool() {
        T *backing_ptr = new T[num_to_alloc];
        for (size_t i = 0; i < num_to_alloc; i++) pool.push_back(&backing_ptr[i]);
        backing_ptr_vec.push_back(backing_ptr);
        num_to_alloc *= 2;
    }

    T *alloc() {
        if (pool.empty()) extend_pool();
        T *ret = pool.back();
        pool.pop_back();
        return ret;
    }

    void free(T *t) { pool.push_back(t); }

    AppMemPool() {}
    ~AppMemPool() {
        for (T *ptr : backing_ptr_vec) delete[] ptr;
    }
};

// eRPC context passed between request and responses
class AppContext {
    public:
        struct {
            // This is maintained between calls to GetReqBuf and SendRequest
            // to reduce copying
            req_tag_t *crt_req_tag;
            // Request tags used for RPCs exchanged with the servers
            AppMemPool<req_tag_t> req_tag_pool;
            boost::unordered_map<TransportReceiver *, boost::unordered_map<std::pair<uint8_t, uint8_t>, int>> sessions;
        } client;

        struct {
            // current req_handle
#if MULTIPLE_ACTIVE_REQUESTS
            boost::unordered_map<uint64_t, erpc::ReqHandle *> req_handles;
            uint64_t req_handle_idx = 0;
#endif
            erpc::ReqHandle *req_handle;
            std::vector<long> latency_get;
            std::vector<long> latency_prepare;
            std::vector<long> latency_commit;
            TransportReceiver *receiver = nullptr;
        } server;

        // common to both servers and clients
        erpc::Rpc<erpc::CTransport> *rpc = nullptr;
};

// RPC stub for eRPC protocol
class FastTransport : public Transport
{
public:
    FastTransport(const network::Configuration &config,
                  std::string &ip,
                  int nthreads,
                  uint8_t nr_req_types,
                  uint8_t phy_port,
                  uint8_t numa_node,
                  uint8_t id);
    virtual ~FastTransport();
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

    // The port of the fast NIC
    uint8_t phy_port;

    // Number of server threads
    int nthreads;

    // numa node on which this transport thread is running
    uint8_t numa_node;

    // used as the RPC id, must be unique per transport thread
    uint8_t id;

    // Index of the receiver (if -1 then the receiver is a client that
    // does not get requests, otherwise it is a server from the configuration)
    int receiverIdx;

    // Nexus object
    erpc::Nexus *nexus;

    struct FastTransportTimerInfo
    {
        FastTransport *transport;
        timer_callback_t cb;
        event *ev;
        int id;
    };

    event_base *eventBase;
    std::vector<event *> signalEvents;
    AppContext *c;
    bool stop = false;

    // TODO: find some other method to deal with timeouts (hidden in eRPC?)
    uint64_t lastTimerId;
    using timers_map = std::map<int, FastTransportTimerInfo *>;
    timers_map timers;
    std::mutex timers_lock;

//    void OnTimer(FastTransportTimerInfo *info);
//    static void SocketCallback(evutil_socket_t fd, short what, void *arg);
//    static void TimerCallback(evutil_socket_t fd, short what, void *arg);
//    static void LogCallback(int severity, const char *msg);
//    static void FatalCallback(int err);
//    static void SignalCallback(evutil_socket_t fd, short what, void *arg);
};

// A basic session management handler that expects successful responses
static void basic_sm_handler(int session_num, erpc::SmEventType sm_event_type,
                      erpc::SmErrType sm_err_type, void *_context) {

    auto *c = static_cast<AppContext *>(_context);

    Assert(sm_err_type == erpc::SmErrType::kNoError);
    //  "SM response with error " + erpc::sm_err_type_str(sm_err_type));

    if (!(sm_event_type == erpc::SmEventType::kConnected ||
          sm_event_type == erpc::SmEventType::kDisconnected)) {
        throw std::runtime_error("Received unexpected SM event.");
    }

    Debug("Rpc %u: Session number %d %s. Error %s. "
            "Time elapsed = %.3f s.\n",
            c->rpc->get_rpc_id(), session_num,
            erpc::sm_event_type_str(sm_event_type).c_str(),
            erpc::sm_err_type_str(sm_err_type).c_str(),
            c->rpc->sec_since_creation());
}

} // namespace network

#endif  // _NETWORK_FASTTRANSPORT_H_
