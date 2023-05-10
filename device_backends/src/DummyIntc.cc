// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DummyIntc.h"

#include "NumericAddressedInterruptDispatcher.h"
#include <nlohmann/json.hpp>

namespace ChimeraTK {

  DummyIntc::DummyIntc(NumericAddressedBackend* backend, std::vector<uint32_t> const& controllerID,
      ChimeraTK::RegisterPath const& module)
  : InterruptControllerHandler(backend, controllerID), _module(module) {}

  void DummyIntc::handle() {
    if(!_activeInterrupts) {
      // FIXME: creating here is not accordiong to spec. It might throw a logic error!
      _activeInterrupts = _backend->getRegisterAccessor<uint32_t>(_module / "active_ints", 1, 0, {});
    }
    assert(_activeInterrupts);
    _activeInterrupts->read();
    std::cout << "This is handle(): activeInterrupts is " << _activeInterrupts->accessData(0) << std::endl;
    for(uint32_t i = 0; i < 32; ++i) {
      if(_activeInterrupts->accessData(0) & 0x1U << i) {
        try {
          _dispatchers.at(i)->trigger();
        }
        catch(std::out_of_range&) {
          throw ChimeraTK::runtime_error("DummyIntc reports unknown active interrupt " + std::to_string(i));
        }
      }
    }
  }

  void DummyIntc::activateImpl() {
    if(!_activeInterrupts) {
      // FIXME: creating here is not accordiong to spec. It might throw a logic error!
      _activeInterrupts = _backend->getRegisterAccessor<uint32_t>(_module / "active_ints", 1, 0, {});
    }
  }

  std::unique_ptr<DummyIntc> DummyIntc::create(
      NumericAddressedBackend* backend, std::vector<uint32_t> const& controllerID, std::string const& desrciption) {
    auto jdescription = nlohmann::json::parse(desrciption);
    auto module = jdescription["module"].get<std::string>();
    return std::make_unique<DummyIntc>(backend, controllerID, module);
  }

} // namespace ChimeraTK
