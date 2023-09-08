// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioAccess.h"

#include "Exception.h"
#include <sys/mman.h>

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <poll.h>

namespace ChimeraTK {

  UioAccess::UioAccess(const std::string& deviceFilePath) : _deviceFilePath(deviceFilePath.c_str()) {
  }

  UioAccess::~UioAccess() {
    close();
  }

  void UioAccess::open() {
    std::string fileName = _deviceFilePath.filename().string();
    //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    _deviceKernelBase = reinterpret_cast<void*>(readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/addr"));
    _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/size");
    _lastInterruptCount = readUint32FromFile("/sys/class/uio/" + fileName + "/event");

    _mmio.emplace(_deviceFilePath.c_str(), _deviceMemSize);
    _opened = true;
  }

  void UioAccess::close() {
    if(!_opened) {
      _mmio.reset();
      _opened = false;
    }
  }

  void UioAccess::read(uint64_t map, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map use absolute bus addresses.
    //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Read request exceeds device memory region");
    }

    _mmio->read(address, data, sizeInBytes);
  }

  void UioAccess::write(uint64_t map, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map use absolute bus addresses.
    //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Write request exceeds device memory region");
    }

    _mmio->write(address, data, sizeInBytes);
  }

  uint32_t UioAccess::waitForInterrupt(int timeoutMs) {
    // Represents the total interrupt count since system uptime.
    uint32_t totalInterruptCount = 0;
    // Will hold the number of new interrupts
    uint32_t occurredInterruptCount = 0;

    struct pollfd pfd = {_mmio->getFile().fd(), POLLIN, 0};

    int ret = poll(&pfd, 1, timeoutMs);

    if(ret >= 1) {
      // No timeout, start reading
      ret = ::read(_mmio->getFile().fd(), &totalInterruptCount, sizeof(totalInterruptCount));

      if(ret != (ssize_t)sizeof(totalInterruptCount)) {
        throw ChimeraTK::runtime_error("UIO - Reading interrupt failed: " + std::string(std::strerror(errno)));
      }

      // Prevent overflow of interrupt count value
      occurredInterruptCount = subtractUint32OverflowSafe(totalInterruptCount, _lastInterruptCount);
      _lastInterruptCount = totalInterruptCount;
    }
    else if(ret == 0) {
      // Timeout
      occurredInterruptCount = 0;
    }
    else {
      throw ChimeraTK::runtime_error("UIO - Waiting for interrupt failed: " + std::string(std::strerror(errno)));
    }
    return occurredInterruptCount;
  }

  void UioAccess::clearInterrupts() {
    uint32_t unmask = 1;
    ssize_t ret = ::write(_mmio->getFile().fd(), &unmask, sizeof(unmask));

    if(ret != (ssize_t)sizeof(unmask)) {
      throw ChimeraTK::runtime_error("UIO - Waiting for interrupt failed: " + std::string(std::strerror(errno)));
    }
  }

  std::string UioAccess::getDeviceFilePath() {
    return _deviceFilePath.string();
  }

  uint32_t UioAccess::subtractUint32OverflowSafe(uint32_t minuend, uint32_t subtrahend) {
    if(subtrahend > minuend) {
      return minuend +
          (uint32_t)(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - static_cast<uint64_t>(subtrahend));
    }
    else {
      return minuend - subtrahend;
    }
  }

  uint32_t UioAccess::readUint32FromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> value;
      inputFile.close();
    }
    return (uint32_t)value;
  }

  uint64_t UioAccess::readUint64HexFromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> std::hex >> value;
      inputFile.close();
    }
    return value;
  }
} // namespace ChimeraTK
