#pragma once

#include <array>

#include "error.hpp"
#include "usb/arraymap.hpp"
#include "usb/classdriver/base.hpp"
#include "usb/endpoint.hpp"
#include "usb/setupdata.hpp"

namespace usb {

class ClassDriver;
class Device {
public:
    virtual ~Device();
    virtual Error ControlIn(EndpointID ep_id, SetupData setup_data,
                            void* buf, int len, ClassDriver* issuer);
    virtual Error ControlOut(EndpointID ep_id, SetupData setup_data,
                             const void* buf, int len, ClassDriver* issuer);
    virtual Error InterruptIn(EndpointID ep_id, void* buf, int len);
    virtual Error InterruptOut(EndpointID ep_id, void* buf, int len);

    Error StartInitialize();
    bool IsInitialized() { return is_initialized_; }
    EndpointConfig* EndpointConfigs() { return ep_configs_.data(); }
    Error OnEndpointsConfigured();
    int NumEndpointConfigs() { return num_ep_configs_; }

    uint8_t* Buffer() { return buf_.data(); }
protected:
    Error OnControlCompleted(EndpointID ep_id, SetupData setup_data,
                             const void* buf, int len);
    Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len);
private:
    std::array<ClassDriver*, 16> class_drivers_{};
    std::array<uint8_t, 16> buf_{};
    uint8_t num_configurations_;
    uint8_t config_index_;
    bool is_initialized_ = false;
    int initialize_phase_ = 0;
    std::array<EndpointConfig, 16> ep_configs_;
    int num_ep_configs_;

    Error InitializePhase1(const uint8_t* buf, int len);
    Error InitializePhase2(const uint8_t* buf, int len);
    Error InitializePhase3(uint8_t config_value);
    ArrayMap<SetupData, ClassDriver*, 4> event_waiters_{};
};

Error GetDescriptor(Device& dev, EndpointID ep_id, uint8_t desc_type, uint8_t desc_index,
                    void* buf, int len, bool debug = false);
Error SetConfiguration(Device& dev, EndpointID ep_id,
                       uint8_t config_value, bool debug = false);
}