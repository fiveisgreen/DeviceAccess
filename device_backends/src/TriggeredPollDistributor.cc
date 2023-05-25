// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggeredPollDistributor.h"

#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  TriggeredPollDistributor::TriggeredPollDistributor(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> interruptID,
      std::shared_ptr<InterruptControllerHandler> parent)
  : _id(std::move(interruptID)), _backend(backend), _controllerHandlerFactory(controllerHandlerFactory),
    _parent(parent) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::trigger(VersionNumber version) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    if(!_isActive) return;

    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* polledAsyncVariable = dynamic_cast<PolledAsyncVariable*>(var.second.get());
        assert(polledAsyncVariable);
        polledAsyncVariable->fillSendBuffer(version);
        var.second->send(); // send function from  the AsyncVariable base class
      }

      auto controllerHandler = _controllerHandler.lock();
      if(controllerHandler) {
        controllerHandler->handle(version);
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::activate(VersionNumber version) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* polledAsyncVariable = dynamic_cast<PolledAsyncVariable*>(var.second.get());
        assert(polledAsyncVariable);
        polledAsyncVariable->fillSendBuffer(version);
        var.second->activateAndSend(); // function from  the AsyncVariable base class
      }
      auto controllerHandler = _controllerHandler.lock();
      if(controllerHandler) {
        controllerHandler->activate(version);
      }
      _isActive = true;
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<TriggeredPollDistributor> TriggeredPollDistributor::getNestedPollDistributor(
      std::vector<uint32_t> const& interruptID) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    auto controllerHandler = _controllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _controllerHandlerFactory->createInterruptControllerHandler(
          _id, boost::dynamic_pointer_cast<TriggeredPollDistributor>(shared_from_this()));
      _controllerHandler = controllerHandler;
    }
    return controllerHandler->getTriggerPollDistributorRecursive(interruptID, _isActive);
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::postDeactivateHook() {
    auto controllerHandler = _controllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->deactivate();
    }
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::postSendExceptionHook(const std::exception_ptr& e) {
    auto controllerHandler = _controllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->sendException(e);
    }
  }

} // namespace ChimeraTK
