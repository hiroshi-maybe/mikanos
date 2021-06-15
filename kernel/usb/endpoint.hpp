#pragma once

namespace usb {

enum class EndpointType {
    kControl = 0,
    kIsochronous = 1,
    kBulk = 2,
    kInterrupt = 3,
};

class EndpointID {
public:
    constexpr EndpointID() : addr_{0} {}
    constexpr EndpointID(const EndpointID& ep_id) : addr_{ep_id.addr_} {}
    explicit constexpr EndpointID(int addr) : addr_{addr} {}
    constexpr EndpointID(int ep_num, bool dir_in) : addr_{ep_num << 1 | dir_in} {}

    int Address() const { return addr_; }
    int Number() const { return addr_ >> 1; }
    bool IsIn() const { return addr_ & 1; }
private:
    int addr_;
};

constexpr EndpointID kDefaultControlPipeID{0, true};

struct EndpointConfig {
    EndpointID ep_id;
    EndpointType ep_type;
    int max_packet_size;

    /** cycle to control this endpoint (125*2^(interval-1) micro seconds) */
    int interval;
  };

}