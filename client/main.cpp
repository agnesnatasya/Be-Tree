// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <csignal>

#include "network/configuration.hpp"
#include "network/grpctransport.hpp"
#include "common/gflags.hpp"
#include "storage_client.hpp"

#include <boost/thread/thread.hpp>

// Generated class by proto
using betree::GetNodeIdReply;
using betree::GetNodeIdRequest;
using betree::Storage;
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

using namespace std;
string const RPC = "G";

// Implmentation of the client's stub implementation
class StorageClientStub {}

void client_thread_func()
{
    std::string local_uri = config.GetServerAddress(FLAGS_serverIndex).host;
    // TODO: get rid of the hardcoded number of request types

    network::Transport;
    if (RPC == "E")
    {
        // TODO: Connect to transport object created by the server
        // Need to be blocking
    }
    else if (RPC == "G")
    {
        StorageClientStub client_stub(grpc::CreateChannel(
            "localhost:50051", grpc::InsecureChannelCredentials()));

        // Spawn reader thread that loops indefinitely
        std::thread thread_ = std::thread(&StorageClientStub::AsyncCompleteRpc, &client_stub);

        for (int i = 0; i < 100; i++)
        {
            client_stub.GetNodeId();
        }

        thread_.join(); // blocks forever
    }
}

int main(int argc, char **argv)
{

    client_thread_func();

    return 0;
}
