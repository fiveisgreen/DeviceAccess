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
  : _id(std::move(interruptID)), _backend(backend), _interruptControllerHandlerFactory(controllerHandlerFactory),
    _parent(std::move(parent)) {}

  template<typename DistributorType>
  boost::shared_ptr<DistributorType> TriggerDistributor::getDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    std::lock_guard<std::recursive_mutex> crerationLock(_creationMutex);

    if(interruptID.size() == 1) {
      // return the distributor from this instance, not a nested one

      boost::weak_ptr<DistributorType>* weakDistributor{
          nullptr}; // Cannot create a reference here, but references in the "if constexpr" scope are not seen outside

      // return the distributor from this instance, not a nested one
      if constexpr(std::is_same<DistributorType, TriggeredPollDistributor>::value) {
        weakDistributor = &_pollDistributor;
      }
      else if constexpr(std::is_same<DistributorType, VariableDistributor<ChimeraTK::Void>>::value) {
        weakDistributor = &_variableDistributor;
      }
      else {
        throw ChimeraTK::logic_error("TriggerDistributor::getDistributorRecursive(): Wrong template parameter.");
      }

      auto distributor = weakDistributor->lock();
      if(!distributor) {
        distributor = boost::make_shared<DistributorType>(
            boost::dynamic_pointer_cast<DeviceBackendImpl>(_backend->shared_from_this()), _id, shared_from_this());
        *weakDistributor = distributor;
        if(_backend->isAsyncReadActive()) {
          distributor->activate({});
        }
      }
      return distributor;
    }
    // get a nested poll distributor
    auto controllerHandler = _interruptControllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _interruptControllerHandlerFactory->createInterruptControllerHandler(_id, shared_from_this());
      _interruptControllerHandler = controllerHandler;
    }

    return controllerHandler->getDistributorRecursive<DistributorType>({++interruptID.begin(), interruptID.end()});
  }

  boost::shared_ptr<TriggeredPollDistributor> TriggerDistributor::getPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<TriggeredPollDistributor>(interruptID);
  }

  boost::shared_ptr<VariableDistributor<ChimeraTK::Void>> TriggerDistributor::getVariableDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<VariableDistributor<ChimeraTK::Void>>(interruptID);
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
