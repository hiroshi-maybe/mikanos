#pragma once

#include <array>
#include <cstdio>

class Error {
public:
    enum Code {
        kSuccess,
        kFull,
        kNoEnoughMemory,
        kIndexOutOfRange,
        kInvalidSlotID,
        kInvalidEndpointNumber,
        kTransferRingNotSet,
        kAlreadyAllocated,
        kNotImplemented,
        kInvalidDescriptor,
        kNoCorrespondingSetupStage,
        kTransferFailed,
        kInvalidPhase,
        kUnknownXHCISpeedID,
        kNoWaiter,
        kNoPCIMSI,
        kLastOfCode,
    };
private:
    static constexpr std::array code_names_{
        "kSuccess",
        "kFull",
        "kNoEnoughMemory",
        "kIndexOutOfRange",
        "kInvalidSlotID",
        "kInvalidEndpointNumber",
        "kTransferRingNotSet",
        "kAlreadyAllocated",
        "kNotImplemented",
        "kInvalidDescriptor",
        "kNoCorrespondingSetupStage",
        "kTransferFailed",
        "kInvalidPhase",
        "kUnknownXHCISpeedID",
        "kNoWaiter",
        "kNoPCIMSI",
    };
    static_assert(Error::Code::kLastOfCode == code_names_.size());
public:
    Error(Code code, const char* file, int line) : code_{code}, line_{line}, file_{file} {}

    operator bool() const {
        return this->code_ != kSuccess;
    }

    const char* Name() const {
        return code_names_[static_cast<int>(this->code_)];
    }

    const char* File() const {
        return this->file_;
    }

    int Line() const {
        return this->line_;
    }
private:
    Code code_;
    int line_;
    const char* file_;
};

#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

template <class T>
struct WithError {
  T value;
  Error error;
};
