// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "InterruptControllerHandler.h"

#include "Axi4_Intc.h"
#include "DummyIntc.h"
#include "TriggerDistributor.h"
#include "TriggeredPollDistributor.h"

#include <tuple>

namespace ChimeraTK {
  //*********************************************************************************************************************/
  InterruptControllerHandlerFactory::InterruptControllerHandlerFactory(DeviceBackendImpl* backend) : _backend(backend) {
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
  boost::shared_ptr<InterruptControllerHandler> InterruptControllerHandlerFactory::createInterruptControllerHandler(
      std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent) {
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
    return creatorFunctionIter->second(this, controllerID, description, parent);
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<DeviceBackendImpl> InterruptControllerHandlerFactory::getBackend() {
    return boost::dynamic_pointer_cast<DeviceBackendImpl>(_backend->shared_from_this());
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<TriggeredPollDistributor> InterruptControllerHandler::getTriggerPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    // assert(false); // FIXME: needs container lock!
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());

    // we can't use try_emplace because the map contains weak pointers
    boost::shared_ptr<TriggerDistributor> distributor;
    auto distributorIter = _distributors.find(interruptID.front());
    if(distributorIter == _distributors.end()) {
      distributor = boost::make_shared<TriggerDistributor>(
          _backend.get(), _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
      _distributors[interruptID.front()] = distributor;
      if(_backend->isAsyncReadActive()) {
        distributor->activate({});
      }
    }
    else {
      distributor = distributorIter->second.lock();
      if(!distributor) {
        distributor = boost::make_shared<TriggerDistributor>(
            _backend.get(), _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
        if(_backend->isAsyncReadActive()) {
          distributor->activate({});
        }
      }
    }
    return distributor->getPollDistributorRecursive(interruptID);
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<VariableDistributor<ChimeraTK::Void>> InterruptControllerHandler::getVariableDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    // assert(false); // FIXME: needs container lock!
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());

    // we can't use try_emplace because the map contains weak pointers
    boost::shared_ptr<TriggerDistributor> distributor;
    auto distributorIter = _distributors.find(interruptID.front());
    if(distributorIter == _distributors.end()) {
      distributor = boost::make_shared<TriggerDistributor>(
          _backend.get(), _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
      _distributors[interruptID.front()] = distributor;
      if(_backend->isAsyncReadActive()) {
        distributor->activate({});
      }
    }
    else {
      distributor = distributorIter->second.lock();
      if(!distributor) {
        distributor = boost::make_shared<TriggerDistributor>(
            _backend.get(), _controllerHandlerFactory, qualifiedInterruptId, shared_from_this());
        if(_backend->isAsyncReadActive()) {
          distributor->activate({});
        }
      }
    }
    return distributor->getVariableDistributorRecursive(interruptID);
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::activate(VersionNumber version) {
    for(auto& dispatcherIter : _distributors) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        // FIXME: this is not thread-safe
        dispatcher->activate(version);
      }
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::sendException(const std::exception_ptr& e) {
    for(auto& dispatcherIter : _distributors) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        dispatcher->sendException(e);
      }
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::deactivate() {
    for(auto& dispatcherIter : _distributors) {
      auto dispatcher = dispatcherIter.second.lock();
      if(dispatcher) {
        dispatcher->deactivate();
      }
    }
  }

} // namespace ChimeraTK
