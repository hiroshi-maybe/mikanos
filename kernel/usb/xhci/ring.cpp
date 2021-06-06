#include "usb/xhci/ring.hpp"

#include "usb/memory.hpp"

namespace usb::xhci {

Ring::~Ring() {
    if (buf_ != nullptr) FreeMem(buf_);
}

Error Ring::Initialize(size_t buf_size) {
    if (buf_ != nullptr) FreeMem(buf_);

    cycle_bit_ = true;
    write_index_ = 0;
    buf_size_ = buf_size;

    buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
    if (buf_ == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
    memset(buf_, 0, buf_size_ * sizeof(TRB));

    return MAKE_ERROR(Error::kSuccess);
}

Error EventRing::Initialize(
    size_t buf_size, InterrupterRegisterSet* interrupter)
{
    if (buf_ != nullptr) FreeMem(buf_);

    cycle_bit_ = true;
    buf_size_ = buf_size;
    interrupter_ = interrupter;

    buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
    if (buf_ == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
    memset(buf_, 0, buf_size_ * sizeof(TRB));

    erst_ = AllocArray<EventRingSegmentTableEntry>(1, 64, 64 * 1024);
    if (erst_ == nullptr) {
        FreeMem(buf_);
        return MAKE_ERROR(Error::kNoEnoughMemory);
    }
    memset(erst_, 0, 1 * sizeof(EventRingSegmentTableEntry));

    erst_[0].bits.ring_segment_base_address = reinterpret_cast<uint64_t>(buf_);
    erst_[0].bits.ring_segment_size = buf_size_;

    ERSTSZ_Bitmap erstsz = interrupter_->ERSTSZ.Read();
    erstsz.SetSize(1);
    interrupter_->ERSTSZ.Write(erstsz);

    WriteDequeuePointer(&buf_[0]);

    ERSTBA_Bitmap erstba = interrupter_->ERSTBA.Read();
    erstba.SetPointer(reinterpret_cast<uint64_t>(erst_));
    interrupter_->ERSTBA.Write(erstba);

    return MAKE_ERROR(Error::kSuccess);
}

void EventRing::WriteDequeuePointer(TRB* p) {
    auto erdp = interrupter_->ERDP.Read();
    erdp.SetPointer(reinterpret_cast<uint64_t>(p));
    interrupter_->ERDP.Write(erdp);
}

}