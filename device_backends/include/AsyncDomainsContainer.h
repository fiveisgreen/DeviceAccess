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
    explicit AsyncDomainsContainer(DeviceBackendImpl* backend);
    ~AsyncDomainsContainer();

    std::map<KeyType, boost::shared_ptr<AsyncDomain>> asyncDomains;

   protected:
    DeviceBackendImpl* _backend;
    void distributeExceptions();
    std::thread _distributorThread;
    class StopThread {};
  };

  /*******************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::distributeExceptions() {
    std::string exceptionMessage;
    while(true) {
      // block to wait until setException has been called
      try {
        _startExceptionDistribution.pop_wait(exceptionMessage);
      }
      catch(StopThread&) {
        return;
      }

      try {
        throw ChimeraTK::runtime_error(exceptionMessage);
      }
      catch(...) {
        for(auto& keyAndDomain : asyncDomains) {
          keyAndDomain.second->sendException(std::current_exception());
        }
      }

      _isSendingExceptions = false;
    }
  }

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::AsyncDomainsContainer(DeviceBackendImpl* backend)
  : AsyncDomainsContainerBase(backend) {
    _distributorThread = std::thread([&] { distributeExceptions(); });
  }

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::~AsyncDomainsContainer() {
    try {
      try {
        throw StopThread();
      }
      catch(...) {
        _startExceptionDistribution.push_overwrite_exception(std::current_exception());
      }
      // Now we can join the thread.
      _distributorThread.join();
    }
    catch(std::system_error& e) {
      // Destructors must not throw. All exceptions that can occur here are system errors, which only show up if there
      // is no hope anyway. All we can do it terminate.
      std::cerr << "Unrecovarable system error in ~AsyncDomainsContainer(): " << e.what() << " !!! TERMINATING !!!"
                << std::endl;
      std::terminate();
    }

    // Unblock a potentially waiting open call
    _isSendingExceptions = false;
  }
} // namespace ChimeraTK
