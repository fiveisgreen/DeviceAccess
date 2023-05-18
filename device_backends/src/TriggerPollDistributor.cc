// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggerPollDistributor.h"

#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  TriggerPollDistributor::TriggerPollDistributor(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> interruptID,
      std::shared_ptr<InterruptControllerHandler> parent)
  : _id(std::move(interruptID)), _backend(backend), _controllerHandlerFactory(controllerHandlerFactory),
    _parent(parent) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  VersionNumber TriggerPollDistributor::trigger() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    if(!_isActive) return ver;

    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* polledAsyncVariable = dynamic_cast<PolledAsyncVariable*>(var.second.get());
        assert(polledAsyncVariable);
        polledAsyncVariable->fillSendBuffer(ver);
        var.second->send(); // send function from  the AsyncVariable base class
      }

      auto controllerHandler = _controllerHandler.lock();
      if(controllerHandler) {
        controllerHandler->handle();
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }

    return ver;
  }

  //*********************************************************************************************************************/
  VersionNumber TriggerPollDistributor::activate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* polledAsyncVariable = dynamic_cast<PolledAsyncVariable*>(var.second.get());
        assert(polledAsyncVariable);
        polledAsyncVariable->fillSendBuffer(ver);
        var.second->activateAndSend(); // function from  the AsyncVariable base class
      }
      auto controllerHandler = _controllerHandler.lock();
      if(controllerHandler) {
        controllerHandler->activate();
      }
      _isActive = true;
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }

    return ver;
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<TriggerPollDistributor> TriggerPollDistributor::getNestedPollDistributor(
      std::vector<uint32_t> const& interruptID) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    auto controllerHandler = _controllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _controllerHandlerFactory->createInterruptControllerHandler(_id);
      _controllerHandler = controllerHandler;
    }
    return controllerHandler->getTriggerPollDistributorRecursive(interruptID);
  }

  //*********************************************************************************************************************/
  void TriggerPollDistributor::postDeactivateHook() {
    auto controllerHandler = _controllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->deactivate();
    }
  }

  //*********************************************************************************************************************/
  void TriggerPollDistributor::postSendExceptionHook(const std::exception_ptr& e) {
    auto controllerHandler = _controllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->sendException(e);
    }
  }

} // namespace ChimeraTK
