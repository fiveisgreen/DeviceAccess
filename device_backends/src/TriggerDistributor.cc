// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggerDistributor.h"

#include "InterruptControllerHandler.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"

namespace ChimeraTK {

  TriggerDistributor::TriggerDistributor(DeviceBackendImpl* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> interruptID,
      boost::shared_ptr<InterruptControllerHandler> parent)
  : _id(interruptID), _backend(backend), _interruptControllerHandlerFactory(controllerHandlerFactory),
    _parent(std::move(parent)) {}

  boost::shared_ptr<TriggeredPollDistributor> TriggerDistributor::getPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    std::lock_guard<std::recursive_mutex> crerationLock(_creationMutex);

    if(interruptID.size() == 1) {
      // return the poll distributor from this instance, not a nested one
      auto pollDistributor = _pollDistributor.lock();
      if(!pollDistributor) {
        pollDistributor = boost::make_shared<TriggeredPollDistributor>(
            boost::dynamic_pointer_cast<DeviceBackendImpl>(_backend->shared_from_this()), _id, shared_from_this());
        _pollDistributor = pollDistributor;
        if(_backend->isAsyncReadActive()) {
          pollDistributor->activate({});
        }
      }
      return pollDistributor;
    }
    // get a nested poll distributor
    auto controllerHandler = _interruptControllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _interruptControllerHandlerFactory->createInterruptControllerHandler(_id, shared_from_this());
      _interruptControllerHandler = controllerHandler;
    }
    return controllerHandler->getTriggerPollDistributorRecursive({++interruptID.begin(), interruptID.end()});
  }

  boost::shared_ptr<VariableDistributor<ChimeraTK::Void>> TriggerDistributor::getVariableDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    std::lock_guard<std::recursive_mutex> crerationLock(_creationMutex);

    if(interruptID.size() == 1) {
      // return the variable distributor from this instance, not a nested one
      auto variableDistributor = _variableDistributor.lock();
      if(!variableDistributor) {
        variableDistributor = boost::make_shared<VariableDistributor<ChimeraTK::Void>>(
            boost::dynamic_pointer_cast<DeviceBackendImpl>(_backend->shared_from_this()), _id, shared_from_this());
        _variableDistributor = variableDistributor;
        if(_backend->isAsyncReadActive()) {
          variableDistributor->activate({});
        }
      }
      return variableDistributor;
    }
    // get a nested variable distributor
    auto controllerHandler = _interruptControllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _interruptControllerHandlerFactory->createInterruptControllerHandler(_id, shared_from_this());
      _interruptControllerHandler = controllerHandler;
    }
    return controllerHandler->getVariableDistributorRecursive({++interruptID.begin(), interruptID.end()});
  }

  void TriggerDistributor::trigger(VersionNumber version) {
    if(!_backend->isAsyncReadActive()) {
      return;
    }
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->trigger(version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->handle(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(version);
    }
  }

  void TriggerDistributor::activate(VersionNumber version) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->activate(version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->activate(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->activate(version);
    }
  }

  void TriggerDistributor::deactivate() {
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->deactivate();
    }
  }

  void TriggerDistributor::sendException(const std::exception_ptr& e) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->sendException(e);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->sendException(e);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->sendException(e);
    }
  }

} // namespace ChimeraTK
