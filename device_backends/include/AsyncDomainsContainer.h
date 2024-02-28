// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "AsyncDomainsContainerBase.h"
#include "DeviceBackendImpl.h"

#include <ChimeraTK/cppext/semaphore.hpp>

#include <map>
#include <thread>

namespace ChimeraTK {

  template<typename KeyType>
  class AsyncDomainsContainer : public AsyncDomainsContainerBase {
   public:
    explicit AsyncDomainsContainer(DeviceBackendImpl* backend) : _backend(backend) {}
    ~AsyncDomainsContainer();

    std::map<KeyType, boost::shared_ptr<AsyncDomain>> asyncDomains;

    bool isSendingExceptions() { return _isSendingExceptions; }
    void sendExceptions();

   protected:
    std::atomic_bool _isSendingExceptions{false};
    DeviceBackendImpl* _backend;
    cppext::semaphore _startExceptionDistribution;
    void distributeExceptions();
    std::thread _distributorThread;
  };

  /*******************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::sendExceptions() {
    if(_isSendingExceptions) {
      // this check also ensures that the semaphore is reset because it is done before clearing the flag
      throw ChimeraTK::logic_error(
          "AsyncDomainsContainer::sendExceptions() called before previous distribution was ready.");
    }
    _isSendingExceptions = true;

    assert(!_startExceptionDistribution.is_ready());
    _startExceptionDistribution.unlock();
  }

  /*******************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::distributeExceptions() {
    // block to wait until setException has been called
    _startExceptionDistribution.wait_and_reset();

    try {
      throw ChimeraTK::runtime_error(_backend->getActiveExceptionMessage());
    }
    catch(ChimeraTK::runtime_error& e) {
      for(auto& keyAndDomain : asyncDomains) {
        keyAndDomain.second->sendException(e);
      }
    }

    _isSendingExceptions = false;
  }

} // namespace ChimeraTK
