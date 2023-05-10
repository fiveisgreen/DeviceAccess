// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "InterruptControllerHandler.h"

#include "Axi4_Intc.h"
#include "DummyIntc.h"
#include "NumericAddressedInterruptDispatcher.h"

#include <tuple>

namespace ChimeraTK {
  //*********************************************************************************************************************/
  InterruptControllerHandlerFactory::InterruptControllerHandlerFactory(NumericAddressedBackend* backend)
  : _backend(backend) {
    // we already know about the build-in handlers
    _creatorFunctions["AXI4_INTC"] = Axi4_Intc::create;
    _creatorFunctions["dummy"] = DummyIntc::create;
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandlerFactory::addInterruptController(
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
    return creatorFunctionIter->second(_backend, controllerID, description);
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::addInterrupt(std::vector<uint32_t> const& interruptID) {
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());
    auto [dispatcherIter, isNew] = _dispatchers.try_emplace(
        interruptID.front(), boost::make_shared<NumericAddressedInterruptDispatcher>(_backend, qualifiedInterruptId));
    auto& dispatcher = dispatcherIter->second; // a map iterator is a pair of key/value
    if(interruptID.size() > 1) {
      dispatcher->addNestedInterrupt({++interruptID.begin(), interruptID.end()});
    }
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<NumericAddressedInterruptDispatcher> const& InterruptControllerHandler::getInterruptDispatcher(
      uint32_t interruptNumber) const {
    return _dispatchers.at(interruptNumber);
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::activateInterruptDispatchers() {
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
  void InterruptControllerHandler::deactivateInterruptDispatchers() {
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->deactivate();
    }
  }

} // namespace ChimeraTK
