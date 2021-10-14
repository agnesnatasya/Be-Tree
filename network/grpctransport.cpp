/**
 * RPC stub implementation for gRPC protocol
 * Should handle async calls and message buffers
 */
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "network/grpctransport.hpp"

// Generated class by proto
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using betree::Storage;
using betree::GetNodeIdReply;
using betree::GetNodeIdRequest;

namespace network {

static std::mutex grpctransport_lock;
static volatile bool grpctransport_initialized = false;

GrpcTransport::GrpcTransport(
    const network::Configuration &config,
    std::string &ip,
    int nthreads,
    uint8_t nr_req_types,
    uint8_t id) {

    // The first thread to grab the lock initializes the transport
    grpctransport_lock.lock();
    if (!grpctransport_initialized) {
        std::string server_address("0.0.0.0:50051");

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        std::unique_ptr<ServerCompletionQueue> cq_ = builder.AddCompletionQueue();
        std::unique_ptr<Server> server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        grpctransport_initialized = true;
    }
    grpctransport_lock.unlock();

    // Server's main loop.
    HandleRpcs();
}

GrpcTransport::~GrpcTransport() {
}

void GrpcTransport::Register(TransportReceiver *receiver, int receiverIdx) {
}

void GrpcTransport::Run() {

}
void GrpcTransport::Wait() {
}

void GrpcTransport::Stop() {
    Debug("Stopping transport!");
    stop = true;
}

bool GrpcTransport::SendRequestToServer(TransportReceiver *src, uint8_t reqType, uint32_t serverIdx, uint8_t dstRpcIdx, size_t msgLen) {}
bool GrpcTransport::SendRequestToAllServers(TransportReceiver *src, uint8_t reqType, uint8_t dstRpcIdx, size_t msgLen) {}
bool GrpcTransport::SendResponse(uint64_t reqHandleIdx, size_t msgLen) {}
bool GrpcTransport::SendResponse(size_t msgLen) {}
char GrpcTransport::*GetRequestBuf(size_t reqLen, size_t respLen) {}
int GrpcTransport::GetSession(TransportReceiver *src, uint8_t replicaIdx, uint8_t dstRpcIdx) {}

} // namespace network
