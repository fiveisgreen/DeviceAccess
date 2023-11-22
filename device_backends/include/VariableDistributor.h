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
    VariableDistributor(boost::shared_ptr<DeviceBackendImpl> backend, std::vector<uint32_t> interruptID,
        boost::shared_ptr<TriggerDistributor> parent);

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(AccessorInstanceDescriptor const& descriptor);

    void activate(VersionNumber version) override;
    void postSendExceptionHook([[maybe_unused]] const std::exception_ptr& e) override {}

    void distribute(VersionNumber version);

    std::vector<uint32_t> _id;
    boost::shared_ptr<TriggerDistributor> _parent;
    typename NDRegisterAccessor<SourceType>::Buffer _sourceBuffer;
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/
  template<typename SourceType>
  VariableDistributor<SourceType>::VariableDistributor(boost::shared_ptr<DeviceBackendImpl> backend,
      std::vector<uint32_t> interruptID, boost::shared_ptr<TriggerDistributor> parent)
  : AsyncAccessorManager(backend), _id(std::move(interruptID)), _parent(std::move(parent)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  template<typename SourceType>
  void VariableDistributor<SourceType>::activate(VersionNumber version) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    for(auto& var : _asyncVariables) {
      var.second->fillSendBuffer(version);
      var.second->activateAndSend(); // function from  the AsyncVariable base class
    }
  }

  //*********************************************************************************************************************/
  template<typename SourceType>
  void VariableDistributor<SourceType>::distribute(VersionNumber version) {
    // Returning here is an optimisation. The deactivation can happen any time after checking the flag, and the timing
    // races are resolved by holding the lock when filling the queue
    if(!_backend->isAsyncReadActive()) {
      return;
    }

    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    for(auto& var : _asyncVariables) {
      var.second->fillSendBuffer(version);
      var.second->send(); // function from  the AsyncVariable base class
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  class VoidAsyncVariable : public AsyncVariableImpl<UserType> {
   public:
    VoidAsyncVariable() : AsyncVariableImpl<UserType>(1, 1) {}

    void fillSendBuffer(VersionNumber const& version) final;

    unsigned int getNumberOfChannels() override { return 1; }
    unsigned int getNumberOfSamples() override { return 1; }
    const std::string& getUnit() override { return _emptyString; }
    const std::string& getDescription() override { return _emptyString; }
    bool isWriteable() override { return false; }

   protected:
    std::string _emptyString{};
  };

  //*********************************************************************************************************************/

  // currently only for void
  template<>
  template<typename UserType>
  std::unique_ptr<AsyncVariable> VariableDistributor<ChimeraTK::Void>::createAsyncVariable(
      [[maybe_unused]] AccessorInstanceDescriptor const& descriptor) {
    // for the full implementation
    // - extract size from catalogue and instance descriptor
    // - if active fill buffer from input buffer (might be tricky)
    return std::make_unique<VoidAsyncVariable<UserType>>();
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void VoidAsyncVariable<UserType>::fillSendBuffer(VersionNumber const& version) {
    this->_sendBuffer.versionNumber = version;
  }

} // namespace ChimeraTK
