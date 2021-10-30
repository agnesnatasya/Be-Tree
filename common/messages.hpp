#ifndef _NETWORK_MESSAGES_H_
#define _NETWORK_MESSAGES_H_

// Request types
namespace requestType {
    constexpr uint8_t getNodeId{3};
}

struct nodeid_request_t {
    uint64_t req_nr;
};

struct nodeid_response_t {
    uint64_t req_nr;
    uint64_t id;
};

#endif  /* _NETWORK_MESSAGES_H_ */
