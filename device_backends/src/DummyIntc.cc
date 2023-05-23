// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DummyIntc.h"

#include "TriggerPollDistributor.h"
#include <nlohmann/json.hpp>

namespace ChimeraTK {

  DummyIntc::DummyIntc(DeviceBackend* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
      std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerPollDistributor> parent,
      ChimeraTK::RegisterPath const& module)
  : InterruptControllerHandler(backend, controllerHandlerFactory, controllerID, parent), _module(module) {
    _activeInterrupts = _backend->getRegisterAccessor<uint32_t>(_module / "active_ints", 1, 0, {});
    if(!_activeInterrupts->isReadable()) {
      throw ChimeraTK::runtime_error("DummyIntc: Handshake register not readable: " + _activeInterrupts->getName());
    }
  }

  void DummyIntc::handle() {
    _activeInterrupts->read();
    for(uint32_t i = 0; i < 32; ++i) {
      if(_activeInterrupts->accessData(0) & 0x1U << i) {
        try {
          auto dispatcher = _dispatchers.at(i).lock();
          if(dispatcher) {
            dispatcher->trigger();
            std::cout << "triggered " << i << std::endl;
          }
          else {
            std::cout << "Warning, not triggering because weak pointer did not lock for interrupt " << i << std::endl;
          }
        }
        catch(std::out_of_range&) {
          throw ChimeraTK::runtime_error("DummyIntc reports unknown active interrupt " + std::to_string(i));
        }
      }
    }
  }

  std::unique_ptr<DummyIntc> DummyIntc::create(DeviceBackend* backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      std::string const& desrciption, boost::shared_ptr<TriggerPollDistributor> parent) {
    auto jdescription = nlohmann::json::parse(desrciption);
    auto module = jdescription["module"].get<std::string>();
    return std::make_unique<DummyIntc>(backend, controllerHandlerFactory, controllerID, parent, module);
  }

} // namespace ChimeraTK
