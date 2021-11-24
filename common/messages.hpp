#ifndef _NETWORK_MESSAGES_H_
#define _NETWORK_MESSAGES_H_

// Request types
const uint8_t getNodeIdReqType = 1;
const uint8_t evictNodeReqType = 2;

struct nodeid_request_t {
    uint64_t req_nr;
};

struct nodeid_response_t {
    uint64_t req_nr;
    uint64_t id;
};

struct evictnode_request_t {
    uint64_t req_nr;
    uint64_t node_id;
};

struct evictnode_response_t {
    uint64_t req_nr;
    bool success;
};


#endif  /* _NETWORK_MESSAGES_H_ */
