// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "AsyncNDRegisterAccessor.h"
#include "DeviceBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "TriggerDistributor.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"
#include "VersionNumber.h"

#include <functional>

namespace ChimeraTK {

  template<typename TargetType, typename BackendDataType>
  class AsyncDomainImpl : public AsyncDomain {
   public:
    explicit AsyncDomainImpl(
        std::function<boost::shared_ptr<TargetType>(boost::shared_ptr<AsyncDomain>)> creatorFunction,
        DeviceBackend* backend)
    : _creatorFunction(creatorFunction), _backend(backend) {}

    void distribute(BackendDataType data, VersionNumber version);
    void activate(BackendDataType data, VersionNumber version);
    void deactivate();
    void sendException(const std::exception_ptr& e) noexcept override;

    template<typename UserDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

   protected:
    // Everything in this class is protected by the mutex from the AsyncDomain base class.
    boost::weak_ptr<TargetType> _target;

    std::function<boost::shared_ptr<TargetType>(boost::shared_ptr<AsyncDomain>)> _creatorFunction;

    // FIXME: Make a weak pointer out of this -> No, this will go away. Info is in creator function
    DeviceBackend* _backend;
    // boost::weak_ptr<DeviceBackend> _backend;

    // Data to resolve a race condition (see distribute and activate)
    BackendDataType _notDistributedData;
    VersionNumber _notDistributedVersion{nullptr};
  };

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomainImpl<TargetType, BackendDataType>::distribute(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // Store the data. We might need it later if the data in activate is older due to a race condition
      _notDistributedData = data;
      _notDistributedVersion = version;
      return;
    }

    auto target = _target.lock();
    if(!target) {
      return;
    }

    target->distribute(data, version);
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomainImpl<TargetType, BackendDataType>::activate(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    _isActive = true;

    auto target = _target.lock();
    if(!target) {
      return;
    }

    if(version >= _notDistributedVersion) {
      target->activate(data, version);
    }
    else {
      // due to a race condition, data has already been tried to
      target->activate(_notDistributedData, _notDistributedVersion);
    }
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomainImpl<TargetType, BackendDataType>::deactivate() {
    std::lock_guard l(_mutex);

    _isActive = false;
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  void AsyncDomainImpl<TargetType, BackendDataType>::sendException(const std::exception_ptr& e) noexcept {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // don't send exceptions if async read is off
      return;
    }

    _isActive = false;
    auto target = _target.lock();
    if(!target) {
      return;
    }

    target->sendException(e);
  }

  //******************************************************************************************************************/

  template<typename TargetType, typename BackendDataType>
  template<typename UserDataType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> AsyncDomainImpl<TargetType, BackendDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard l(_mutex);

    auto target = _target.lock();
    if(!target) {
      target = _creatorFunction(shared_from_this());
      _target = target;
    }

    if constexpr(std::is_same<TargetType, VariableDistributor<UserDataType>>::value) {
      return target->subscribe(name, numberOfWords, wordOffsetInRegister, flags);
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
      const auto& primaryDistributor = target;
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
