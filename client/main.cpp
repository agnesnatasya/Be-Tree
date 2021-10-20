// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <csignal>

#include "network/configuration.hpp"
#include "network/transport.hpp"

#if IS_DEV
#include "network/simtransport.hpp"
#else
#include "network/fasttransport.hpp"
#endif

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

// Implmentation of the client's stub implementation
class StorageClientStub
{
}

void
client_thread_func()
{
    std::string local_uri = config.GetServerAddress(FLAGS_serverIndex).host;
    // TODO: get rid of the hardcoded number of request types

    network::Transport;
    if (FLAGS_serverIndex)
    {
        // TODO: Connect to simulated transport
    }
    
}

int main(int argc, char **argv)
{

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_isDev == "")
    {
        FLAGS_isDev = false;
    }

    client_thread_func();

    return 0;
}