// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Axi4_Intc.h"

#include "TriggerPollDistributor.h"

namespace ChimeraTK {

  void Axi4_Intc::handle() {
    // Stupid testing implementation that always triggers all children
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->trigger();
    }
  }

  std::unique_ptr<Axi4_Intc> Axi4_Intc::create(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      std::string desrciption) {
    std::ignore = desrciption;
    return std::make_unique<Axi4_Intc>(backend, controllerHandlerFactory, controllerID);
  }

} // namespace ChimeraTK
