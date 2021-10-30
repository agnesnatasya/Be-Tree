// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
#ifndef _NETWORK_CONFIGURATION_H_
#define _NETWORK_CONFIGURATION_H_

//#include "replication/common/viewstamp.h"

#include <fstream>
#include <stdbool.h>
#include <string>
#include <vector>

using std::string;

namespace network {

struct ServerAddress
{
    string host;
    string port;
    ServerAddress(const string &host, const string &port);
    bool operator==(const ServerAddress &other) const;
    inline bool operator!=(const ServerAddress &other) const {
        return !(*this == other);
    }
    bool operator<(const ServerAddress &other) const;
    bool operator<=(const ServerAddress &other) const {
        return *this < other || *this == other;
    }
    bool operator>(const ServerAddress &other) const {
        return !(*this <= other);
    }
    bool operator>=(const ServerAddress &other) const {
        return !(*this < other);
    }
};


class Configuration
{
public:
    Configuration();
    Configuration(const Configuration &c);
    Configuration(int n,
//                  int f,
                  std::vector<ServerAddress> servers,
                  ServerAddress *multicastAddress = nullptr);
    Configuration(std::ifstream &file);
    virtual ~Configuration();
    ServerAddress GetServerAddress(int idx) const;
    const ServerAddress *multicast() const;
//    int GetLeaderIndex(view_t view) const;
//    int QuorumSize() const;
//    int FastQuorumSize() const;
    bool operator==(const Configuration &other) const;
    inline bool operator!=(const Configuration &other) const {
        return !(*this == other);
    }
    bool operator<(const Configuration &other) const;
    bool operator<=(const Configuration &other) const {
        return *this < other || *this == other;
    }
    bool operator>(const Configuration &other) const {
        return !(*this <= other);
    }
    bool operator>=(const Configuration &other) const {
        return !(*this < other);
    }

public:
    int n;                      // number of servers
//    int f;                      // number of failures tolerated
private:
    std::vector<ServerAddress> servers;
    ServerAddress *multicastAddress;
    bool hasMulticast;
};

}      // namespace network


namespace std {
template <> struct hash<network::ServerAddress>
{
    size_t operator()(const network::ServerAddress & x) const
        {
            return hash<string>()(x.host) * 37 + hash<string>()(x.port);
        }
};
}

namespace std {
template <> struct hash<network::Configuration>
{
    size_t operator()(const network::Configuration & x) const
        {
            size_t out = 0;
            out = x.n * 37;
            for (int i = 0; i < x.n; i++) {
                out *= 37;
                out += hash<network::ServerAddress>()(x.GetServerAddress(i));
            }
            return out;
        }
};
}

#endif  /* _NETWORK_CONFIGURATION_H_ */
