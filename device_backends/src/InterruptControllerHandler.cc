// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "InterruptControllerHandler.h"

#include "Axi4_Intc.h"
#include "DummyIntc.h"
#include "TriggerPollDistributor.h"

#include <tuple>

namespace ChimeraTK {
  //*********************************************************************************************************************/
  InterruptControllerHandlerFactory::InterruptControllerHandlerFactory(DeviceBackend* backend) : _backend(backend) {
    // we already know about the build-in handlers
    _creatorFunctions["AXI4_INTC"] = Axi4_Intc::create;
    _creatorFunctions["dummy"] = DummyIntc::create;
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandlerFactory::addControllerDescription(
      std::vector<uint32_t> const& controllerID, std::string const& name, std::string const& description) {
    _controllerDescriptions[controllerID] = {name, description};
  }

  //*********************************************************************************************************************/
  std::unique_ptr<InterruptControllerHandler> InterruptControllerHandlerFactory::createInterruptControllerHandler(
      std::vector<uint32_t> const& controllerID) {
    assert(!controllerID.empty());
    std::string name, description;
    try {
      std::tie(name, description) = _controllerDescriptions.at(controllerID);
    }
    catch(std::out_of_range&) {
      std::string idAsString;
      for(auto i : controllerID) {
        idAsString += std::to_string(i) + ":";
      }
      idAsString.pop_back(); // remove last ":"
      throw ChimeraTK::logic_error("Unknown interrupt controller ID " + idAsString);
    }
    auto creatorFunctionIter = _creatorFunctions.find(name);
    if(creatorFunctionIter == _creatorFunctions.end()) {
      throw ChimeraTK::logic_error("Unknown interrupt controller type \"" + name + "\"");
    }
    return creatorFunctionIter->second(_backend, this, controllerID, description);
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<TriggerPollDistributor> InterruptControllerHandler::getTriggerPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    // assert(false); // FIXME: needs container lock!
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());
    auto [dispatcherIter, isNew] = _dispatchers.try_emplace(interruptID.front(),
        boost::make_shared<TriggerPollDistributor>(
            _backend, _controllerHandlerFactory, qualifiedInterruptId, shared_from_this()));
    auto& dispatcher = dispatcherIter->second; // a map iterator is a pair of key/value
    if(interruptID.size() == 1) {
      return dispatcher;
    }
    return dispatcher->getNestedPollDistributor({++interruptID.begin(), interruptID.end()});
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::activate() {
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->activate();
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::sendException(const std::exception_ptr& e) {
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->sendException(e);
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::deactivate() {
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->deactivate();
    }
  }

} // namespace ChimeraTK
