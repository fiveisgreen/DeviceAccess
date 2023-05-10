// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyIntc.h"

#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  void DummyIntc::handle() {
    // Stupid testing implementation that always triggers all children
    for(auto& dispatcher : _dispatchers) {
      dispatcher.second->trigger();
    }
  }

  std::unique_ptr<DummyIntc> DummyIntc::create(
      NumericAddressedBackend* backend, std::vector<uint32_t> const& controllerID, std::string const& desrciption) {
    std::ignore = desrciption;
    return std::make_unique<DummyIntc>(backend, controllerID);
  }

} // namespace ChimeraTK
