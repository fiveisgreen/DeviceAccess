// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AsyncDomainsContainerBase.h"

#include "DeviceBackendImpl.h"

namespace ChimeraTK {
  bool AsyncDomainsContainerBase::isSendingExceptions() {
    return _isSendingExceptions;
  }

  void AsyncDomainsContainerBase::sendExceptions() {
    if(_isSendingExceptions) {
      throw ChimeraTK::logic_error(
          "AsyncDomainsContainer::sendExceptions() called before previous distribution was ready.");
    }
    _isSendingExceptions = true;

    _startExceptionDistribution.push(_backend->getActiveExceptionMessage());
  }

} // namespace ChimeraTK
