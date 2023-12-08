// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "Exception.h"
#include <condition_variable>
#include <shared_mutex>

#include <ChimeraTK/cppext/finally.hpp>

#include <atomic>
#include <list>
#include <mutex>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * DeviceBackendImpl implements some basic functionality which should be available for all backends. This is required
   * to allow proper decorator patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl : public DeviceBackend {
   public:
    bool isOpen() override { return _opened; }

    bool isConnected() final {
      std::cerr << "Removed function DeviceBackendImpl::isConnected() called." << std::endl;
      std::cerr << "Do not use. This function has no valid meaning." << std::endl;
      std::terminate();
    }

    MetadataCatalogue getMetadataCatalogue() const override { return {}; }

    bool isAsyncReadActive() noexcept final {
      std::shared_lock lock(_asyncIsActiveMutex);
      return _asyncIsActive;
    }

    /**
     * Execute the lambda function under the shared lock if asynchronous read is active.
     */
    template<typename MY_LAMBDA>
    void executeIfAsyncActive(MY_LAMBDA executeMe);

    /** Execute under the exclusive lock and then deactivate the active flag.
     *  The code is only executed if async read is active, and afterwards
     *  the flag is deactivated.
     */
    template<typename MY_LAMBDA>
    void executeAndDeactivateAsync(MY_LAMBDA executeMe);

    /** Set the asyncIsActive flag to true under the exclusive lock.
     */
    void setAsyncIsActive();

   protected:
    /** flag if device is opened */
    std::atomic<bool> _opened{false};

   private:
    std::shared_mutex _asyncIsActiveMutex;
    bool _asyncIsActive{false};
    bool _deactivationInProgress{false}; // also protected by the _asyncIsActiveMutex
    std::condition_variable_any _deactivationDoneConditionVar;
  };

  /********************************************************************************************************************/

  template<typename MY_LAMBDA>
  void DeviceBackendImpl::executeIfAsyncActive(MY_LAMBDA executeMe) {
    std::shared_lock lock(_asyncIsActiveMutex);
    if(_asyncIsActive) {
      executeMe();
    }
  }

  /********************************************************************************************************************/

  template<typename MY_LAMBDA>
  void DeviceBackendImpl::executeAndDeactivateAsync(MY_LAMBDA executeMe) {
    // We cannot hold the lock the whole time. This would be cause lock order inversion because
    // there are some container locks which must be held while checking the _asyncIsActive flag.
    // The same container log is acquired in the lambda function when distributing exceptions, so we cannot hold the
    // lock when doing so.

    // On the other hand, we must prevent activation while the deactivation is still going on. Hence we introduce an
    // additional variable that deactivation is going on, and make the activation wait, using a condition variable
    // together with that flag.

    // Whatever happens in here, make sure the _deavtivationInProgress is turned off at the end (by using cppext::finally)
    auto _ = cppext::finally([&] {
      { // lock scope
        std::unique_lock lock(_asyncIsActiveMutex);
        _deactivationInProgress = false;
      }
      _deactivationDoneConditionVar.notify_all();
    });

    {
      std::unique_lock lock(_asyncIsActiveMutex);
      // FIXME: is this the correct behaviour?
      if(!_asyncIsActive) {
        return;
      }
      _asyncIsActive = false;
      _deactivationInProgress = true;
    }

    executeMe();
    // the cppext::finally is executed here, setting back the _deactivationInProgess flags
  }

  /********************************************************************************************************************/

  inline void DeviceBackendImpl::setAsyncIsActive() {
    std::unique_lock lock(_asyncIsActiveMutex);

    // wait for an ongoing deactivation to finish
    _deactivationDoneConditionVar.wait(lock, [&] { return !_deactivationInProgress; });

    _asyncIsActive = true;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
