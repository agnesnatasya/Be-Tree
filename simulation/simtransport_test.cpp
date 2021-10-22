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
#include "client/storage_client.hpp"

#include <boost/thread/thread.hpp>

using namespace std;

void client_thread_func(network::Configuration &config)
{
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

    client_thread_func(config);

    return 0;
}
