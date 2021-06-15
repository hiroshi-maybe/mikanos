#include "usb/xhci/trb.hpp"

namespace usb::xhci {

const std::array<const char*, 64> kTRBTypeToName{
    "Reserved",                             // 0
    "Normal",
    "Setup Stage",
    "Data Stage",
    "Status Stage",
    "Isoch",
    "Link",
    "EventData",
    "No-Op",                                // 8
    "Enable Slot Command",
    "Disable Slot Command",
    "Address Device Command",
    "Configure Endpoint Command",
    "Evaluate Context Command",
    "Reset Endpoint Command",
    "Stop Endpoint Command",
    "Set TR Dequeue Pointer Command",       // 16
    "Reset Device Command",
    "Force Event Command",
    "Negotiate Bandwidth Command",
    "Set Latency Tolerance Value Command",
    "Get Port Bandwidth Command",
    "Force Header Command",
    "No Op Command",
    "Reserved",                             // 24
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Transfer Event",                       // 32
    "Command Completion Event",
    "Port Status Change Event",
    "Bandwidth Request Event",
    "Doorbell Event",
    "Host Controller Event",
    "Device Notification Event",
    "MFINDEX Wrap Event",
    "Reserved",                             // 40
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Vendor Defined",                       // 48
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",                       // 56
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
    "Vendor Defined",
};

}