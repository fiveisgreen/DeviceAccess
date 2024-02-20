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

    /** Deactivate the async active flag and execute the lambda. The function
     *  guarantees that activateAsyncRead() will wait until the lambda has finished.
     *  The code is only executed if async read is active.
     */
    template<typename MY_LAMBDA>
    void deactivateAsyncAndExecute(MY_LAMBDA executeMe);

    /** Set the asyncIsActive flag to true under the exclusive lock.
     */
    void setAsyncIsActive();

    /**
     * Function to be (optionally) implemented by backends if additional actions are needed when switching to the
     * exception state.
     */
    virtual void setExceptionImpl() noexcept {}

    /**
     * Function to be called by backends when needing to check for an active exception. If an active exception is found,
     * the appropriate ChimeraTK::runtime_error is thrown by this function.
     */
    void checkActiveException() final;

    void setException(const std::string& message) noexcept final;

    bool isFunctional() const noexcept final;

    std::string getActiveExceptionMessage() noexcept;

   protected:
    /** Backends should call this function at the end of a (successful) open() call.*/
    void setOpenedAndClearException() noexcept;

    /** flag if backend is opened */
    std::atomic<bool> _opened{false};

   private:
    std::shared_mutex _asyncIsActiveMutex;
    bool _asyncIsActive{false};
    bool _deactivationInProgress{false}; // also protected by the _asyncIsActiveMutex
    std::condition_variable_any _deactivationDoneConditionVar;

    /** flag if backend is in an exception state */
    std::atomic<bool> _hasActiveException{false};

    /**
     *  message for the current exception, if _hasActiveException is true. Access is protected by
     * _mx_activeExceptionMessage
     */
    std::string _activeExceptionMessage;

    /** mutex to protect access to _activeExceptionMessage */
    std::mutex _mx_activeExceptionMessage;
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

  // This function is rather often called and hence implemented as inline in the header for performance reasons.
  inline bool DeviceBackendImpl::isFunctional() const noexcept {
    if(!_opened) return false;
    if(_hasActiveException) return false;
    return true;
  }

  /********************************************************************************************************************/

  template<typename MY_LAMBDA>
  void DeviceBackendImpl::deactivateAsyncAndExecute(MY_LAMBDA executeMe) {
    // We cannot hold the lock the whole time. This would be cause lock order inversion because
    // there are some container locks which must be held while checking the _asyncIsActive flag.
    // The same container log is acquired in the lambda function when distributing exceptions, so we cannot hold the
    // lock when doing so.

    // On the other hand, we must prevent activation while the deactivation (sending exceptions) is still going on.
    // Hence we introduce an additional variable that deactivation is going on, and make the activation wait, using a
    // condition variable together with that flag.

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

  // This function is rather often called and hence implemented as inline in the header for performance reasons.
  inline void DeviceBackendImpl::checkActiveException() {
    if(_hasActiveException) {
      std::lock_guard<std::mutex> lk(_mx_activeExceptionMessage);
      throw ChimeraTK::runtime_error(_activeExceptionMessage);
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
