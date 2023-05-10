// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedInterruptDispatcher.h"

#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  NumericAddressedInterruptDispatcher::NumericAddressedInterruptDispatcher(
      NumericAddressedBackend* backend, std::vector<uint32_t> const& interruptID)
  : _id(interruptID), _backend(backend) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::trigger() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    if(!_isActive) return ver;

    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(var.second.get());
        assert(numericAddressAsyncVariable);
        numericAddressAsyncVariable->fillSendBuffer(ver);
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
  VersionNumber NumericAddressedInterruptDispatcher::activate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(var.second.get());
        assert(numericAddressAsyncVariable);
        numericAddressAsyncVariable->fillSendBuffer(ver);
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
  void NumericAddressedInterruptDispatcher::addNestedInterrupt(std::vector<uint32_t> const& interruptID) {
    if(!_controllerHandler) {
      _controllerHandler = _backend->_interruptControllerHandlerFactory.createInterruptControllerHandler(_id);
    }
    _controllerHandler->addInterrupt(interruptID);
  }
  //*********************************************************************************************************************/
  boost::shared_ptr<NumericAddressedInterruptDispatcher> const& NumericAddressedInterruptDispatcher::
      getNestedDispatcher(std::vector<uint32_t> const& interruptID) {
    const auto& firstLevelNestedDispatcher = _controllerHandler->getInterruptDispatcher(interruptID.front());
    if(interruptID.size() == 1) {
      return firstLevelNestedDispatcher;
    }
    return firstLevelNestedDispatcher->getNestedDispatcher({++interruptID.begin(), interruptID.end()});
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::postDeactivateHook() {
    if(_controllerHandler) {
      _controllerHandler->deactivate();
    }
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::postSendExceptionHook(const std::exception_ptr& e) {
    if(_controllerHandler) {
      _controllerHandler->sendException(e);
    }
  }

} // namespace ChimeraTK
