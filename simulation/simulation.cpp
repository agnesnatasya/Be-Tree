// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <csignal>

#include "network/configuration.hpp"
#include "network/transport.hpp"
#include "network/simtransport.hpp"

#include "common/gflags.hpp"
#include "server/storage_server.hpp"
#include "client/storage_client.hpp"

#include <boost/thread/thread.hpp>

using namespace std;

void client_thread_func(StorageClient *sc)
{
    string request;
    nodeid_t result = sc->GetNodeId(0, 0, request);
    cout << "SUCCESS\n";
    cout << result.nodeIdx << "\n";
}

void server_thread_func(network::SimTransport *transport)
{
    transport->Run();
}

int main(int argc, char **argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_configFile == "")
    {
        fprintf(stderr, "option --configFile is required\n");
        return EXIT_FAILURE;
    }

    // Load configuration
    std::ifstream configStream(FLAGS_configFile);
    if (configStream.fail())
    {
        fprintf(stderr, "unable to read configuration file: %s\n", FLAGS_configFile.c_str());
    }
    network::Configuration config(configStream);
    network::SimTransport *transport = new network::SimTransport(config, 0);

    StorageClient *sc = new StorageClient(config, transport);
    StorageServerApp *storageApp = new StorageServerApp();
    StorageServer *ss = new StorageServer(
        config,
        FLAGS_serverIndex,
        transport,
        storageApp
    );

    /* Thread creation */
    std::vector<std::thread> server_thread_arr(1);
    for (uint8_t i = 0; i < 1; i++)
    {
        server_thread_arr[i] = std::thread(server_thread_func, transport);
    }
    std::vector<std::thread> client_thread_arr(100);
    for (uint8_t i = 0; i < 100; i++)
    {
        client_thread_arr[i] = std::thread(client_thread_func, sc);
    }

    /* Blocking join */
    for (auto &client_thread : client_thread_arr)
        client_thread.join();
    for (auto &server_thread : server_thread_arr)
        server_thread.join();

    return 0;
}
