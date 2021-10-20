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

using namespace std;

// Implmentation of the client's stub implementation
class StorageClientStub
{
}

void client_thread_func()
{
    
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