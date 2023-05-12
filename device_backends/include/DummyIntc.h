// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"
#include "NDRegisterAccessor.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  class DummyIntc : public InterruptControllerHandler {
   public:
    explicit DummyIntc(DeviceBackend* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, RegisterPath const& module);
    ~DummyIntc() override = default;

    void handle() override;

    static std::unique_ptr<DummyIntc> create(DeviceBackend*, InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, std::string const& desrciption);

   protected:
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _activeInterrupts;
    RegisterPath _module;
  };

} // namespace ChimeraTK
