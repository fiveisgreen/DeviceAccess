// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "Exception.h"
#include <shared_mutex>

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
    std::unique_lock lock(_asyncIsActiveMutex);
    if(!_asyncIsActive) {
      return;
    }
    executeMe();
    _asyncIsActive = false;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
