#pragma once

#include <string>

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

namespace ChimeraTK {

  // Control (config/status) interface to access the registers of FPGA IPs
  class CtrlIntf : public XdmaIntfAbstract {
    DeviceFile _file;
    void* _mem;

    // Map 4 kB
    static constexpr size_t _mmapSize = 4* 1024;

    volatile int32_t* _reg_ptr(uintptr_t offs) const;
    void _check_range(const std::string access_type, uintptr_t address, size_t nBytes) const;

   public:
    CtrlIntf() = delete;
    CtrlIntf(const std::string& devicePath);
    virtual ~CtrlIntf();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nBytes) override;
  };

} // namespace ChimeraTK
