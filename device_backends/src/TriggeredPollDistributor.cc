// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggeredPollDistributor.h"

#include "TriggerDistributor.h"

namespace ChimeraTK {

  TriggeredPollDistributor::TriggeredPollDistributor(
      std::vector<uint32_t> interruptID, boost::shared_ptr<TriggerDistributor> parent)
  : _id(std::move(interruptID)), _parent(parent) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::trigger(VersionNumber version) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    if(!_isActive) return;

    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        var.second->fillSendBuffer(version);
        var.second->send(); // send function from  the AsyncVariable base class
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
        var.second->fillSendBuffer(version);
        var.second->activateAndSend(); // function from  the AsyncVariable base class
      }
      _isActive = true;
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::postDeactivateHook() {}

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::postSendExceptionHook([[maybe_unused]] const std::exception_ptr& e) {}

} // namespace ChimeraTK
