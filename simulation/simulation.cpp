// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <csignal>
#include <boost/thread/thread.hpp>

#include "network/configuration.hpp"
#include "network/transport.hpp"
#include "network/simtransport.hpp"

#include "server/storage_server.hpp"
#include "client/storage_client.hpp"


using namespace std;

void client_thread_func(StorageClient *sc)
{
    string request;
    nodeid_t result = sc->GetNodeId(0, 0, request);
    cout << "This is the server's node: " << result.nodeIdx << "\n";
}

void server_thread_func(network::SimTransport *transport)
{
    transport->Run();
}

int main(int argc, char **argv)
{
    network::SimConfiguration config;

    /* Client and server creation */
    // It has to be in this order such that
    // the transport object is correctly registered to the server    

    /* Threads creation */
    int servers = 1;
    int clients = 100;

    std::vector<std::thread> server_thread_arr(servers);
    std::vector<network::SimTransport> transport_arr(servers);
    for (uint8_t i = 0; i < servers; i++)
    {
        network::SimTransport *transport = new network::SimTransport(config, 0);
        transport_arr[i] = transport;
        StorageServer *ss = new StorageServer(
            config,
            FLAGS_serverIndex,
            transport,
            new StorageServerApp());
        server_thread_arr[i] = std::thread(server_thread_func, transport);
    }

    std::vector<std::thread> client_thread_arr(clients);
    for (uint8_t i = 0; i < clients; i++)
    {
        int server_idx = 0;
        network::SimTransport *transport = transport_arr[server_idx];
        StorageClient *sc = new StorageClient(config, transport);
        client_thread_arr[i] = std::thread(client_thread_func, sc);
    }

    /* Blocking join */
    for (auto &server_thread : server_thread_arr)
        server_thread.join();

    return 0;
}
