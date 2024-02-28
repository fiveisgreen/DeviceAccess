// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>

namespace ChimeraTK {
  class AsyncDomainsContainerBase {
   public:
    bool isSendingExceptions() { return _isSendingExceptions; }

   protected:
    std::atomic_bool _isSendingExceptions{false};
  };

} // namespace ChimeraTK
