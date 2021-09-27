// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#include "network/configuration.h"

#include <cstring>
#include <stdexcept>
#include <tuple>

namespace network {

ServerAddress::ServerAddress(const string &host, const string &port)
    : host(host), port(port)
{

}

bool
ServerAddress::operator==(const ServerAddress &other) const {
    return ((host == other.host) &&
            (port == other.port));
}

bool
ServerAddress::operator<(const ServerAddress &other) const {
    auto this_t = std::forward_as_tuple(host, port);
    auto other_t = std::forward_as_tuple(other.host, other.port);
    return this_t < other_t;
}

Configuration::Configuration(const Configuration &c)
    : n(c.n), f(c.f), servers(c.servers), hasMulticast(c.hasMulticast)
{
    multicastAddress = NULL;
    if (hasMulticast) {
        multicastAddress = new ServerAddress(*c.multicastAddress);
    }
}

Configuration::Configuration(int n,
                             std::vector<ServerAddress> servers,
                             ServerAddress *multicastAddress)
    : n(n), servers(servers)
{
    if (multicastAddress) {
        hasMulticast = true;
        this->multicastAddress =
            new ServerAddress(*multicastAddress);
    } else {
        hasMulticast = false;
        multicastAddress = NULL;
    }
}

Configuration::Configuration(std::ifstream &file)
{
    //f = -1;
    hasMulticast = false;
    multicastAddress = NULL;

    while (!file.eof()) {
        // Read a line
        string line;
        getline(file, line);;

        // Ignore comments
        if ((line.size() == 0) || (line[0] == '#')) {
            continue;
        }

        // Get the command
        unsigned int t1 = line.find_first_of(" \t");
        string cmd = line.substr(0, t1);

//        if (strcasecmp(cmd.c_str(), "f") == 0) {
//            unsigned int t2 = line.find_first_not_of(" \t", t1);
//            if (t2 == string::npos) {
//                Panic ("'f' configuration line requires an argument");
//            }
//
//            try {
//                f = stoul(line.substr(t2, string::npos));
//            } catch (std::invalid_argument& ia) {
//                Panic("Invalid argument to 'f' configuration line");
//            }
//        } else
        if (strcasecmp(cmd.c_str(), "server") == 0) {
            unsigned int t2 = line.find_first_not_of(" \t", t1);
            if (t2 == string::npos) {
                Panic ("'server' configuration line requires an argument");
            }

            unsigned int t3 = line.find_first_of(":", t2);
            if (t3 == string::npos) {
                Panic("Configuration line format: 'server host:port'");
            }

            string host = line.substr(t2, t3-t2);
            string port = line.substr(t3+1, string::npos);

            servers.push_back(ReplicaAddress(host, port));
//        } else if (strcasecmp(cmd.c_str(), "multicast") == 0) {
//            unsigned int t2 = line.find_first_not_of(" \t", t1);
//            if (t2 == string::npos) {
//                Panic ("'multicast' configuration line requires an argument");
//            }
//
//            unsigned int t3 = line.find_first_of(":", t2);
//            if (t3 == string::npos) {
//                Panic("Configuration line format: 'server host:port'");
//            }
//
//            string host = line.substr(t2, t3-t2);
//            string port = line.substr(t3+1, string::npos);
//
//            multicastAddress = new ServerAddress(host, port);
//            hasMulticast = true;
        } else {
            Panic("Unknown configuration directive: %s", cmd.c_str());
        }
    }

    n = servers.size();
    if (n == 0) {
        Panic("Configuration did not specify any servers");
    }
}

Configuration::~Configuration()
{
    if (hasMulticast) {
        delete multicastAddress;
    }
}

ServerAddress
Configuration::GetServerAddress(int idx) const
{
    return servers[idx];
}

const ServerAddress *
Configuration::multicast() const
{
    if (hasMulticast) {
        return multicastAddress;
    } else {
        return nullptr;
    }
}

bool
Configuration::operator==(const Configuration &other) const
{
    if ((n != other.n) ||
//        (f != other.f) ||
        (servers != other.servers) ||
        (hasMulticast != other.hasMulticast)) {
        return false;
    }

    if (hasMulticast) {
        if (*multicastAddress != *other.multicastAddress) {
            return false;
        }
    }

    return true;
}

bool
Configuration::operator<(const Configuration &other) const {
    auto this_t = std::forward_as_tuple(n, servers, hasMulticast);
    auto other_t = std::forward_as_tuple(other.n, other.servers,
                                         other.hasMulticast);
    if (this_t < other_t) {
        return true;
    } else if (this_t == other_t) {
        if (hasMulticast) {
            return *multicastAddress < *other.multicastAddress;
        } else {
            return false;
        }
    } else {
        // this_t > other_t
        return false;
    }
}

} // namespace network
