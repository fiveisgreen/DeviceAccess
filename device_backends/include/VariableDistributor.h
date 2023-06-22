// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncAccessorManager.h"
#include "InterruptControllerHandler.h"

#include <memory>
#include <mutex>

namespace ChimeraTK {

  template<typename SourceType>
  class VariableDistributor : public AsyncAccessorManager {
   public:
    VariableDistributor(std::vector<uint32_t> interruptID, boost::shared_ptr<TriggerDistributor> parent);

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(
        const boost::shared_ptr<DeviceBackend>& backend, AccessorInstanceDescriptor const& descriptor, bool isActive);

    void activate(VersionNumber version) override;
    void postDeactivateHook() override {}
    void postSendExceptionHook([[maybe_unused]] const std::exception_ptr& e) override {}

    std::vector<uint32_t> _id;
    boost::shared_ptr<TriggerDistributor> _parent;
    typename NDRegisterAccessor<SourceType>::Buffer _sourceBuffer;
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/
  template<typename SourceType>
  VariableDistributor<SourceType>::VariableDistributor(
      std::vector<uint32_t> interruptID, boost::shared_ptr<TriggerDistributor> parent)
  : _id(std::move(interruptID)), _parent(std::move(parent)) {}

  template<typename SourceType>
  void VariableDistributor<SourceType>::activate(VersionNumber version) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    // FIXME: How do I get the initial value into the user buffer?
    // This implementation only works for void
    for(auto& var : _asyncVariables) {
      var.second->fillSendBuffer(version);
      var.second->activateAndSend(); // function from  the AsyncVariable base class
    }
    _isActive = true;
  }

  // currently only for void
  template<>
  template<typename UserType>
  std::unique_ptr<AsyncVariable> VariableDistributor<ChimeraTK::Void>::createAsyncVariable(
      [[maybe_unused]] const boost::shared_ptr<DeviceBackend>& backend,
      [[maybe_unused]] AccessorInstanceDescriptor const& descriptor, [[maybe_unused]] bool isActive) {
    // for the full implementation
    // - extract size from catalogue and instance descriptor
    // - if active fill buffer from input buffer (might be tricky)
    return std::make_unique<AsyncVariableImpl<UserType>>(1, 1);
  }

} // namespace ChimeraTK
