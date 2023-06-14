// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggerDistributor.h"

#include "InterruptControllerHandler.h"
#include "TriggeredPollDistributor.h"
// #include "VariableDistributor.h"

namespace ChimeraTK {

  TriggerDistributor::TriggerDistributor(DeviceBackend* backend,
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
            _backend, _interruptControllerHandlerFactory, _id, shared_from_this());
        _pollDistributor = pollDistributor;
        if(_isActive) {
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
    return controllerHandler->getTriggerPollDistributorRecursive({++interruptID.begin(), interruptID.end()}, _isActive);
  }

  void TriggerDistributor::trigger(VersionNumber version) {
    if(!_isActive) {
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
  }

  void TriggerDistributor::activate(VersionNumber version) {
    _isActive = true;
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->activate(version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->activate(version);
    }
  }

  void TriggerDistributor::deactivate() {
    _isActive = false;
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->deactivate();
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->deactivate();
    }
  }

  void TriggerDistributor::sendException(const std::exception_ptr& e) {
    _isActive = false;
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->sendException(e);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->sendException(e);
    }
  }

} // namespace ChimeraTK
