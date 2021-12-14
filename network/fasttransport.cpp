// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include "debug/message.hpp"
#include "debug/assert.hpp"
#include "network/fasttransport.hpp"

#include <event2/event.h>
#include <event2/thread.h>

#include <memory>
#include <random>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <thread>
#include <sched.h>

#include <numa.h>
#include <boost/fiber/all.hpp>


// TODO: currently, the multiplexing of received responses and requests is delegated to the 
// TransportReceiver, but eRPC does a lot of that (the transport receiver just needs to register
// callbacks)

namespace network {

static std::mutex fasttransport_lock;
static volatile bool fasttransport_initialized = false;

// Function called when we received a response to a
// request we sent on this transport
static void fasttransport_response(void *_context, void *_tag) {
    auto *c = static_cast<AppContext *>(_context);
    auto *rt = reinterpret_cast<req_tag_t *>(_tag);
    Debug("Received respose, reqType = %d", rt->reqType);
    rt->src->ReceiveResponse(rt->reqType,
                            reinterpret_cast<char *>(rt->resp_msgbuf.buf_));
    c->rpc->free_msg_buffer(rt->req_msgbuf);
    c->rpc->free_msg_buffer(rt->resp_msgbuf);
    c->client.req_tag_pool.free(rt);
}

// Function called when the server received a request
// (clients never receive requests, just responses)
static void fasttransport_request(erpc::ReqHandle *req_handle, void *_context, uint8_t reqType) {
    // save the req_handle for when we are in the SendMessage function
    auto *c = static_cast<AppContext *>(_context);
#if MULTIPLE_ACTIVE_REQUESTS
    c->server.req_handles[c->server.req_handle_idx] = req_handle;
    // upcall to the app
    c->server.receiver->ReceiveRequest(c->server.req_handle_idx,
                                reqType,
                                reinterpret_cast<char *>(req_handle->get_req_msgbuf()->buf),
                                reinterpret_cast<char *>(req_handle->pre_resp_msgbuf.buf));
    c->server.req_handle_idx++;
#else
    c->server.req_handle = req_handle;
    // upcall to the app
    c->server.receiver->ReceiveRequest(reqType,
                                reinterpret_cast<char *>(req_handle->get_req_msgbuf()->buf_),
                                reinterpret_cast<char *>(req_handle->pre_resp_msgbuf_.buf_));
#endif
}

// TODO: Hack!!! eRPC changed its API and now we can't get the requType from the reqMsgBuffer
// So we create here a bunch of function macros for every reqType
// We assume maximum 10 reqTypes (1-10); if you need more, add here

#define GENERATE_FUNCTION(reqType) static void fasttransport_request_##reqType(erpc::ReqHandle *req_handle, void *_context) \
                                               { fasttransport_request(req_handle, _context, reqType); }
#define fasttransport_request_(reqType) fasttransport_request_##reqType

GENERATE_FUNCTION(1)
GENERATE_FUNCTION(2)
GENERATE_FUNCTION(3)
GENERATE_FUNCTION(4)
GENERATE_FUNCTION(5)

FastTransport::FastTransport(const network::Configuration &config,
                             std::string &ip,
                             int nthreads,
                             uint8_t nr_req_types,
                             uint8_t phy_port,
                             uint8_t numa_node,
                             uint8_t id)
    : config(config),
      phy_port(phy_port),
      nthreads(nthreads),
      numa_node(numa_node),
      id(id) {

    Assert(numa_node <=  numa_max_node());

    c = new AppContext();

    // The first thread to grab the lock initializes the transport
    fasttransport_lock.lock();
    if (fasttransport_initialized) {
        // Create the event_base to schedule requests
        eventBase = event_base_new();
        evthread_make_base_notifiable(eventBase);
    } else {
        // Setup libevent
        evthread_use_pthreads(); // TODO: do we really need this even
                                 // when we manipulate one eventbase
                                 // per thread?
//        event_set_log_callback(LogCallback);
//        event_set_fatal_callback(FatalCallback);

        // Create the event_base to schedule requests
        eventBase = event_base_new();
        evthread_make_base_notifiable(eventBase);

        // signals must be registered only on one eventBase
        // signalEvents.push_back(evsignal_new(eventBase, SIGTERM,
        //         SignalCallback, this));
        // signalEvents.push_back(evsignal_new(eventBase, SIGINT,
        //         SignalCallback, this));

        for (event *x : signalEvents) {
            event_add(x, NULL);
        }

        fasttransport_initialized = true;
    }

    // Setup eRPC

    // TODO: why sharing one nexus object between threads does not scale?
    // TODO: create one nexus per numa node
    // right now we create one nexus object per thread
    std::string local_uri = ip + ":" + std::to_string(erpc::kBaseSmUdpPort + id);
    nexus = new erpc::Nexus(local_uri, numa_node, 0);
    Warning("Created nexus object with local_uri = %s", local_uri.c_str());

    // register receive handlers
    //for (uint8_t j = 1; j <= nr_req_types; j++) {
    // TODO: Hack!!! For now, just register 5 functions, for 5 different ReqTypes
    nexus->register_req_func(1, fasttransport_request_(1), erpc::ReqFuncType::kForeground);
    nexus->register_req_func(2, fasttransport_request_(2), erpc::ReqFuncType::kForeground);
    nexus->register_req_func(3, fasttransport_request_(3), erpc::ReqFuncType::kForeground);
    nexus->register_req_func(4, fasttransport_request_(4), erpc::ReqFuncType::kForeground);
    nexus->register_req_func(5, fasttransport_request_(5), erpc::ReqFuncType::kForeground);
    //}

    // Create the RPC object
    //c->rpc = new erpc::Rpc<erpc::CTransport> (nexus[numa_node],
    c->rpc = new erpc::Rpc<erpc::CTransport> (nexus,
                                            static_cast<void *>(c),
                                            static_cast<uint8_t>(id),
                                            basic_sm_handler, phy_port);
    c->rpc->retry_connect_on_invalid_rpc_id_ = true;
    fasttransport_lock.unlock();
}

FastTransport::~FastTransport() {
}

// Clients register with receiverIdx of -1
void FastTransport::Register(TransportReceiver *receiver, int receiverIdx) {

    assert(receiverIdx < config.n);

    if (receiverIdx > -1) c->server.receiver = receiver;
    this->receiverIdx = receiverIdx;
}

inline char *FastTransport::GetRequestBuf(size_t reqLen, size_t respLen) {
    // create a new request tag
    if (reqLen == 0)
        reqLen = c->rpc->get_max_data_per_pkt();
    if (respLen == 0)
        respLen = c->rpc->get_max_data_per_pkt();

    c->client.crt_req_tag = c->client.req_tag_pool.alloc();
    c->client.crt_req_tag->req_msgbuf = c->rpc->alloc_msg_buffer_or_die(reqLen);
    c->client.crt_req_tag->resp_msgbuf = c->rpc->alloc_msg_buffer_or_die(respLen);
    return reinterpret_cast<char *>(c->client.crt_req_tag->req_msgbuf.buf_);
}

inline int FastTransport::GetSession(TransportReceiver *src, uint8_t serverIdx, uint8_t dstRpcIdx) {
    auto session_key = std::make_pair(serverIdx, dstRpcIdx);

    const auto iter = c->client.sessions[src].find(session_key);
    if (iter == c->client.sessions[src].end()) {
        // create a new session to the server core
        // use the dafault port from eRPC for control path
        // TODO: pass in the number of numa nodes at the server (in the form of the mapping function)
        //int numa_nodes_at_servers = 2;
        //int numa_nodes_at_servers = 1;
        int session_id = c->rpc->create_session(config.GetServerAddress(serverIdx).host + ":" +
                                       //std::to_string(erpc::kBaseSmUdpPort + dstRpcIdx % numa_nodes_at_servers), dstRpcIdx);
                                       std::to_string(erpc::kBaseSmUdpPort + dstRpcIdx), dstRpcIdx);
        while (!c->rpc->is_connected(session_id)) {
            c->rpc->run_event_loop_once();
        }
        c->client.sessions[src][session_key] = session_id;
        Warning("Opened eRPC session to %s, RPC id: %d", (config.GetServerAddress(serverIdx).host + ":" + std::to_string(erpc::kBaseSmUdpPort)).c_str(), dstRpcIdx);
        return session_id;
    } else {
        return iter->second;
    }
}

// This function assumes the message has already been copied to the
// req_msgbuf
bool FastTransport::SendRequestToServer(TransportReceiver *src,
                                        uint8_t reqType,
                                        uint32_t serverIdx,
                                        uint8_t dstRpcIdx,
                                        size_t msgLen) {
    Assert(serverIdx < config.n);
    int session_id = GetSession(src, serverIdx, dstRpcIdx);

    c->client.crt_req_tag->src = src;
    c->client.crt_req_tag->reqType = reqType;
    c->rpc->resize_msg_buffer(&c->client.crt_req_tag->req_msgbuf, msgLen);
    c->rpc->enqueue_request(session_id, reqType,
                            &c->client.crt_req_tag->req_msgbuf,
                            &c->client.crt_req_tag->resp_msgbuf,
                            fasttransport_response,
                            reinterpret_cast<void *>(c->client.crt_req_tag));

    while (src->Blocked()) {
        c->rpc->run_event_loop_once();
        boost::this_fiber::yield();
    }
    return true;
}

// Sends request to all servers in the configuration, except if the sender is
// one of those servers (as opposed to a client), it doesn't send to itself
bool FastTransport::SendRequestToAllServers(TransportReceiver *src,
                                    uint8_t reqType,
                                    uint8_t dstRpcIdx,
                                    size_t msgLen) {
    c->rpc->resize_msg_buffer(&c->client.crt_req_tag->req_msgbuf, msgLen);

    for (int i = 0; i < config.n; i++) {
        // skip the sending entity
        if (this->receiverIdx == i) continue;
        int session_id = GetSession(src, i, dstRpcIdx);

        if (i == config.n - 1) {
            c->client.crt_req_tag->src = src;
            c->client.crt_req_tag->reqType = reqType;
            c->rpc->enqueue_request(session_id, reqType,
                                &c->client.crt_req_tag->req_msgbuf,
                                &c->client.crt_req_tag->resp_msgbuf,
                                fasttransport_response,
                                reinterpret_cast<void *>(c->client.crt_req_tag));
        } else {
            // need to use different erpc::MsgBuffer per session
            auto *rt = c->client.req_tag_pool.alloc();
            rt->req_msgbuf = c->rpc->alloc_msg_buffer_or_die(msgLen);
            rt->resp_msgbuf = c->rpc->alloc_msg_buffer_or_die(c->rpc->get_max_data_per_pkt());
            rt->reqType = reqType;
            rt->src = src;
            std::memcpy(reinterpret_cast<char *>(rt->req_msgbuf.buf_),
                        reinterpret_cast<char *>(c->client.crt_req_tag->req_msgbuf.buf_), msgLen);
            c->rpc->enqueue_request(session_id, reqType,
                                    &rt->req_msgbuf,
                                    &rt->resp_msgbuf,
                                    fasttransport_response,
                                    reinterpret_cast<void *>(rt));
        }
    }
    if (this->receiverIdx == config.n - 1) {
        // TODO: free the current buffer
    }

    while (src->Blocked()) {
        c->rpc->run_event_loop_once();
        boost::this_fiber::yield();
    }
    return true;
}

// For cases when there are multiple active requests that need response
bool FastTransport::SendResponse(uint64_t reqHandleIdx, size_t msgLen) {
    // we get here from fasttransport_rpc_request
#if MULTIPLE_ACTIVE_REQUESTS
    auto &resp = c->server.req_handles[reqHandleIdx]->pre_resp_msgbuf;
    c->rpc->resize_msg_buffer(&resp, msgLen);
    c->rpc->enqueue_response(c->server.req_handles[reqHandleIdx], &resp);
    c->server.req_handles.erase(reqHandleIdx);
    Debug("Sent response, msgLen = %lu\n", msgLen);
#else
    Panic("Compile fasttransport with MULTIPLE_ACTIVE_REQUESTS true");
#endif
    return true;
}

// Assumes we already put the response in c->server.req_handle->pre_resp_msgbuf
bool FastTransport::SendResponse(size_t msgLen) {
    // we get here from fasttransport_rpc_request
    auto &resp = c->server.req_handle->pre_resp_msgbuf_;
    c->rpc->resize_msg_buffer(&resp, msgLen);
    c->rpc->enqueue_response(c->server.req_handle, &resp);
    Debug("Sent response, msgLen = %lu\n", msgLen);
    return true;
}

void FastTransport::Run() {
    while(!stop) {
        // if (serverIdx == -1)
        //    event_base_loop(eventBase, EVLOOP_ONCE|EVLOOP_NONBLOCK);
        c->rpc->run_event_loop_once();
    }
}

//int FastTransport::Timer(uint64_t ms, timer_callback_t cb) {
//    FastTransportTimerInfo *info = new FastTransportTimerInfo();
//
//    struct timeval tv;
//    tv.tv_sec = ms/1000;
//    tv.tv_usec = (ms % 1000) * 1000;
//
//    timers_lock.lock();
//    uint64_t t_id = lastTimerId;
//    lastTimerId++;
//    timers_lock.unlock();
//
//    info->transport = this;
//    info->id = t_id;
//    info->cb = cb;
//    info->ev = event_new(eventBase, -1, 0,
//                         TimerCallback, info);
//
//    if (info->ev == NULL) {
//        Debug("Error creating new Timer event : %lu", t_id);
//    }
//
//    timers_lock.lock();
//    timers[info->id] = info;
//    timers_lock.unlock();
//
//    int ret = event_add(info->ev, &tv);
//    if (ret != 0) {
//        Debug("Error adding new Timer event to eventbase %lu", t_id);
//    }
//    
//    return info->id;
//}
//
//bool FastTransport::CancelTimer(int id)
//{
//    FastTransportTimerInfo *info = timers[id];
//
//    if (info == NULL) {
//         return false;
//    }
//
//    event_del(info->ev);
//    event_free(info->ev);
//
//    timers_lock.lock();
//    timers.erase(info->id);
//    timers_lock.unlock();
//
//    delete info;
//    
//    return true;
//}
//
//void FastTransport::CancelAllTimers() {
//    Debug("Cancelling all Timers");
//    while (!timers.empty()) {
//        auto kv = timers.begin();
//        CancelTimer(kv->first);
//    }
//}
//
//void FastTransport::OnTimer(FastTransportTimerInfo *info) {
//    timers_lock.lock();
//    timers.erase(info->id);
//    timers_lock.unlock();
//
//    event_del(info->ev);
//    event_free(info->ev);
//
//    info->cb();
//
//    delete info;
//}
//
//void FastTransport::TimerCallback(evutil_socket_t fd, short what, void *arg) {
//    FastTransport::FastTransportTimerInfo *info =
//        (FastTransport::FastTransportTimerInfo *)arg;
//
//    ASSERT(what & EV_TIMEOUT);
//
//    info->transport->OnTimer(info);
//}
//
//void FastTransport::LogCallback(int severity, const char *msg) {
//    Message_Type msgType;
//    switch (severity) {
//    case _EVENT_LOG_DEBUG:
//        msgType = MSG_DEBUG;
//        break;
//    case _EVENT_LOG_MSG:
//        msgType = MSG_NOTICE;
//        break;
//    case _EVENT_LOG_WARN:
//        msgType = MSG_WARNING;
//        break;
//    case _EVENT_LOG_ERR:
//        msgType = MSG_WARNING;
//        break;
//    default:
//        NOT_REACHABLE();
//    }
//
//    _Message(msgType, "libevent", 0, NULL, "%s", msg);
//}
//
//void FastTransport::FatalCallback(int err) {
//    Panic("Fatal libevent error: %d", err);
//}
//
//void FastTransport::SignalCallback(evutil_socket_t fd,
//      short what, void *arg) {
//    Notice("Terminating on SIGTERM/SIGINT");
//    FastTransport *transport = (FastTransport *)arg;
//    //event_base_loopbreak(libeventBase);
//    transport->Stop();
//}
//
void FastTransport::Stop() {
    Debug("Stopping transport!");
    stop = true;
}

} // namespace network
