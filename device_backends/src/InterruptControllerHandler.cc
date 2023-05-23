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
      std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerPollDistributor> parent) {
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
    return creatorFunctionIter->second(_backend, this, controllerID, description, parent);
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<TriggerPollDistributor> InterruptControllerHandler::getTriggerPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID, bool activateIfNew) {
    // assert(false); // FIXME: needs container lock!
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());

    // we can't use try_emplace because the map contains weak pointers
    boost::shared_ptr<TriggerPollDistributor> dispatcher;
    auto dispatcherIter = _dispatchers.find(interruptID.front());
    if(dispatcherIter == _dispatchers.end()) {
      std::cout << "Creating new TriggerPolldistributor level " << qualifiedInterruptId.size() << std::endl;
      dispatcher = boost::make_shared<TriggerPollDistributor>(
          _backend, _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
      _dispatchers[interruptID.front()] = dispatcher;
      if(activateIfNew) {
        dispatcher->activate();
      }
    }
    else {
      dispatcher = dispatcherIter->second.lock();
      if(!dispatcher) {
        std::cout << "Replacing lost TriggerPolldistributor level " << qualifiedInterruptId.size() << std::endl;
        dispatcher = boost::make_shared<TriggerPollDistributor>(
            _backend, _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
        _dispatchers[interruptID.front()] = dispatcher;
        if(activateIfNew) {
          dispatcher->activate();
        }
      }
    }

    if(interruptID.size() == 1) {
      return dispatcher;
    }
    return dispatcher->getNestedPollDistributor({++interruptID.begin(), interruptID.end()});
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::activate() {
    for(auto& dispatcherIter : _dispatchers) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        dispatcher->activate();
      }
      else {
        std::cout << "Warning, not activating because weak pointer did not lock" << std::endl;
      }
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::sendException(const std::exception_ptr& e) {
    for(auto& dispatcherIter : _dispatchers) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        dispatcher->sendException(e);
      }
      else {
        std::cout << "Warning, not sending exception because weak pointer did not lock" << std::endl;
      }
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::deactivate() {
    for(auto& dispatcherIter : _dispatchers) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        dispatcher->deactivate();
      }
      else {
        std::cout << "Warning, not deactivating because weak pointer did not lock" << std::endl;
      }
    }
  }

} // namespace ChimeraTK
