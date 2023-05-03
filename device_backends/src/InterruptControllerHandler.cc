// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "InterruptControllerHandler.h"

#include "Axi4_Intc.h"

#include <tuple>

namespace ChimeraTK {
  InterruptControllerHandlerFactory::InterruptControllerHandlerFactory(NumericAddressedBackend* backend)
  : _backend(backend) {
    // we already know about the build-in handlers
    _creatorFunctions["AXI4_INTC"] = Axi4_Intc::create;
  }

  void InterruptControllerHandlerFactory::addInterruptController(
      std::vector<uint32_t> controllerID, std::string name, std::string description) {
    _controllerDescriptions[controllerID] = {name, description};
  }

  std::unique_ptr<InterruptControllerHandler> InterruptControllerHandlerFactory::createInterruptControllerHandler(
      std::vector<uint32_t> controllerID) {
    std::string name, description;
    std::tie(name, description) = _controllerDescriptions[controllerID];
    return _creatorFunctions[name](_backend, description);
  }

  void InterruptControllerHandler::activateInterrupt(uint32_t interrupt) {
    _dispatchers.try_emplace(interrupt, boost::make_shared<NumericAddressedInterruptDispatcher>());
  }

} // namespace ChimeraTK
