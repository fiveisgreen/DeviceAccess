// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Axi4_Intc.h"

#include "TriggerDistributor.h"

namespace ChimeraTK {

  void Axi4_Intc::handle(VersionNumber version) {
    // Stupid testing implementation that always triggers all children
    for(auto& dispatcherIter : _distributors) {
      auto dispatcher = dispatcherIter.second.lock();
      // The weak pointer might have gone.
      // FIXME: We need a cleanup function which removes the map entry. Otherwise we might
      // be stuck with a bad weak pointer wich is tried in each handle() call.
      if(dispatcher) {
        dispatcher->trigger(version);
      }
    }
  }

  std::unique_ptr<Axi4_Intc> Axi4_Intc::create(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      std::string desrciption, boost::shared_ptr<TriggerDistributor> parent) {
    std::ignore = desrciption;
    return std::make_unique<Axi4_Intc>(backend, controllerHandlerFactory, controllerID, parent);
  }

} // namespace ChimeraTK
