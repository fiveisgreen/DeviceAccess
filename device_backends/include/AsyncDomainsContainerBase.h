// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/cppext/future_queue.hpp>

#include <atomic>

namespace ChimeraTK {
  class DeviceBackendImpl;

  class AsyncDomainsContainerBase {
   public:
    explicit AsyncDomainsContainerBase(DeviceBackendImpl* backend) : _backend(backend) {}
    virtual ~AsyncDomainsContainerBase() = default;
    bool isSendingExceptions();
    void sendExceptions();

   protected:
    std::atomic_bool _isSendingExceptions{false};
    cppext::future_queue<std::string> _startExceptionDistribution{2};
    DeviceBackendImpl* _backend;
  };

} // namespace ChimeraTK
