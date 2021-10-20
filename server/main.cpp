// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <csignal>
#include <numa.h>

#include "network/transport_config.hpp"
#include "network/configuration.hpp"

#if IS_DEV
#include "network/simtransport.hpp"
#else
#include "network/fasttransport.hpp"
#endif

#include "common/gflags.hpp"
#include "storage_server.hpp"

#include <boost/thread/thread.hpp>

using namespace std;

void server_thread_func(StorageServerApp *storageApp,
                        network::Configuration config,
                        uint8_t numa_node, uint8_t thread_id) {
    std::string local_uri = config.GetServerAddress(FLAGS_serverIndex).host;
    // TODO: provide mapping function from thread_id to numa_node
    // for now assume it's round robin
    // TODO: get rid of the hardcoded number of request types
    //int ht_ct = boost::thread::hardware_concurrency();
    network::FastTransport *transport = new network::FastTransport(config,
                                                local_uri,
                                                //FLAGS_numServerThreads,
                                                1,
                                                //ht_ct,
                                                4,
                                                0,
                                                numa_node,
                                                thread_id);
//    last_transport = transport;

    StorageServer *ss = new StorageServer(
      config, FLAGS_serverIndex,
      (network::FastTransport *)transport,
      storageApp);

    transport->Run();
}

//void signal_handler( int signal_num ) {
//   last_transport->Stop();
//   last_replica->PrintStats();
//   global_server->PrintStats();
//
//   // terminate program
//   exit(signal_num);
//}

int
main(int argc, char **argv)
{
//    signal(SIGINT, signal_handler);

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_configFile == "") {
        fprintf(stderr, "option --configFile is required\n");
        return EXIT_FAILURE;
    }

//    if (FLAGS_keysFile == "") {
//        fprintf(stderr, "option --keysFile is required\n");
//        return EXIT_FAILURE;
//    }

    if (FLAGS_serverIndex == -1) {
        fprintf(stderr, "option replicaIndex is required\n");
        return EXIT_FAILURE;
    }

    // Load configuration
    std::ifstream configStream(FLAGS_configFile);
    if (configStream.fail()) {
        fprintf(stderr, "unable to read configuration file: %s\n", FLAGS_configFile.c_str());
    }
    network::Configuration config(configStream);

    if (FLAGS_serverIndex >= config.n) {
        fprintf(stderr, "server index %d is out of bounds; "
                "only %d servers defined\n", FLAGS_serverIndex, config.n);
    }

    // create replica threads
    // bind round robin on the availlable numa nodes
    if (numa_available() == -1) {
        PPanic("NUMA library not available.");
    }

    //int nn_ct = numa_max_node() + 1;
    //int ht_ct = boost::thread::hardware_concurrency()/boost::thread::physical_concurrency(); // number of hyperthreads

    // TODO: start the app on all available cores to regulate frequency boosting
//    int ht_ct = boost::thread::hardware_concurrency();
//      std::vector<std::thread> thread_arr(FLAGS_numServerThreads);
//    std::vector<std::thread> thread_arr(ht_ct);
      std::vector<std::thread> thread_arr(1);

      StorageServerApp *storageApp = new StorageServerApp();

//    for (uint8_t i = 0; i < FLAGS_numServerThreads; i++) {
//    for (uint8_t i = 0; i < ht_ct; i++) {
    for (uint8_t i = 0; i < 1; i++) {
        // thread_arr[i] = std::thread(server_thread_func, server, config, i%nn_ct, i);
        // erpc::bind_to_core(thread_arr[i], i%nn_ct, i/nn_ct);
//        uint8_t numa_node = (i % 4 < 2)?0:1;
//        uint8_t idx = i/4 + (i % 2) * 20;
        uint8_t numa_node = 0;
        uint8_t idx = i;        
        thread_arr[i] = std::thread(server_thread_func, storageApp, config, numa_node, i);
        erpc::bind_to_core(thread_arr[i], numa_node, idx);
    }

    for (auto &thread : thread_arr) thread.join();

    return 0;
}
