// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>

namespace ChimeraTK {
  class DeviceBackendImpl;

  class AsyncDomainsContainerBase {
   public:
    virtual ~AsyncDomainsContainerBase() = default;
    bool isSendingExceptions() { return _isSendingExceptions; }
    virtual void sendExceptions() {}

   protected:
    std::atomic_bool _isSendingExceptions{false};
  };

} // namespace ChimeraTK
