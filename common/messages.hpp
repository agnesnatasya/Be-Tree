#ifndef _NETWORK_MESSAGES_H_
#define _NETWORK_MESSAGES_H_

// Request types
const uint8_t getNodeIdReqType = 1;

struct nodeid_request_t {
    uint64_t req_nr;
};

struct nodeid_response_t {
    uint64_t req_nr;
    uint64_t id;
};

#endif  /* _NETWORK_MESSAGES_H_ */
