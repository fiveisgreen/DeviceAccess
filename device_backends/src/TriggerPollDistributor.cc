// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggerPollDistributor.h"

#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  TriggerPollDistributor::TriggerPollDistributor(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& interruptID)
  : _id(interruptID), _backend(backend), _controllerHandlerFactory(controllerHandlerFactory) {
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

      if(_controllerHandler) {
        _controllerHandler->handle();
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
      if(_controllerHandler) {
        _controllerHandler->activate();
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
  void TriggerPollDistributor::addNestedInterrupt(std::vector<uint32_t> const& interruptID) {
    if(!_controllerHandler) {
      _controllerHandler = _controllerHandlerFactory->createInterruptControllerHandler(_id);
    }
    _controllerHandler->addInterrupt(interruptID);
  }
  //*********************************************************************************************************************/
  boost::shared_ptr<TriggerPollDistributor> const& TriggerPollDistributor::getNestedDispatcher(
      std::vector<uint32_t> const& interruptID) {
    const auto& firstLevelNestedDispatcher = _controllerHandler->getInterruptDispatcher(interruptID.front());
    if(interruptID.size() == 1) {
      return firstLevelNestedDispatcher;
    }
    return firstLevelNestedDispatcher->getNestedDispatcher({++interruptID.begin(), interruptID.end()});
  }

  //*********************************************************************************************************************/
  void TriggerPollDistributor::postDeactivateHook() {
    if(_controllerHandler) {
      _controllerHandler->deactivate();
    }
  }

  //*********************************************************************************************************************/
  void TriggerPollDistributor::postSendExceptionHook(const std::exception_ptr& e) {
    if(_controllerHandler) {
      _controllerHandler->sendException(e);
    }
  }

} // namespace ChimeraTK
