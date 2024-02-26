// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "DeviceBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "TriggerDistributor.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"
#include "VersionNumber.h"

#include <boost/shared_ptr.hpp>

#include <mutex>

namespace ChimeraTK {

  template<typename TargetType, typename BackendDataType>
  class AsyncDomain {
   public:
    explicit AsyncDomain(boost::shared_ptr<TargetType> target, DeviceBackend* backend)
    : _backend(backend), _target(target) {}

    void distribute(BackendDataType data, VersionNumber version);
    void activate(BackendDataType data, VersionNumber version);
    void deactivate();
    void sendException(const std::exception_ptr& e);

    template<typename UserDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    // FIXME: Make a weak pointer out of this
    DeviceBackend* _backend;
    // boost::weak_ptr<DeviceBackend> _backend;

   private:
    // The mutex has to be recursive because an exception can occur within distribute, which is automatically triggering
    // a sendException. This would deadlock with a regular mutex.
    std::recursive_mutex _mutex;
    // Everything in this class is protected by the mutex
    bool _isActive{false};
    boost::shared_ptr<TargetType> _target;

    // Data to resolve a race condition (see distribute and activate)
    BackendDataType _notDistributedData;
    VersionNumber _notDistributedVersion{nullptr};
  };

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomain<TargetType, BackendDataType>::distribute(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // Store the data. We might need it later if the data in activate is older due to a race condition
      _notDistributedData = data;
      _notDistributedVersion = version;
      return;
    }

    _target->distribute(data, version);
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomain<TargetType, BackendDataType>::activate(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    _isActive = true;
    if(version >= _notDistributedVersion) {
      _target->activate(data, version);
    }
    else {
      // due to a race condition, data has already been tried to
      _target->activate(_notDistributedData, _notDistributedVersion);
    }
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomain<TargetType, BackendDataType>::deactivate() {
    std::lock_guard l(_mutex);

    _isActive = false;
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomain<TargetType, BackendDataType>::sendException(const std::exception_ptr& e) {
    std::lock_guard l(_mutex);

    _isActive = false;
    _target->sendException(e);
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  template<typename UserDataType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> AsyncDomain<TargetType, BackendDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard l(_mutex);

    if constexpr(std::is_same<TargetType, VariableDistributor<UserDataType>>::value) {
      return _target->subscribe(name, numberOfWords, wordOffsetInRegister, flags);
    }
    else if constexpr(std::is_same<TargetType, TriggerDistributor>::value) {
      // FIXME: Move this code to the TriggeredPollDistributor!
      // const auto& backendCatalogue = _backend.lock()->getRegisterCatalogue().getImpl();
      auto catalogue = _backend->getRegisterCatalogue(); // need to store the clone you get
      const auto& backendCatalogue = catalogue.getImpl();
      // This code only works for backends which use the NumericAddressedRegisterCatalogue because we need the
      // interrupt description which is specific for those backends and not in the general catalogue.
      // If the cast fails, it will throw an exception.
      auto removeMe = backendCatalogue.getRegister(name);
      const auto& numericCatalogue = dynamic_cast<const NumericAddressedRegisterCatalogue&>(backendCatalogue);
      auto registerInfo = numericCatalogue.getBackendRegister(name);

      // Find the right place in the distribution tree to subscribe
      const auto& primaryDistributor = _target;
      boost::shared_ptr<AsyncAccessorManager> distributor;
      if(registerInfo.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::nodata) {
        distributor = primaryDistributor->getVariableDistributorRecursive(registerInfo.interruptId);
      }
      else {
        distributor = primaryDistributor->getPollDistributorRecursive(registerInfo.interruptId);
      }

      return distributor->template subscribe<UserDataType>(name, numberOfWords, wordOffsetInRegister, flags);
    }

    assert(false); // The code should never end up here.
    return {nullptr};
  }

} // namespace ChimeraTK
