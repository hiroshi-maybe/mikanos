#pragma once

namespace usb {
class Device;

class ClassDriver {
public:
    ClassDriver(Device* dev);
    virtual ~ClassDriver();
private:
    Device* dev_;
};

}