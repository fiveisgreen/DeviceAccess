// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  class Axi4_Intc : public InterruptControllerHandler {
   public:
    explicit Axi4_Intc(DeviceBackend* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID)
    : InterruptControllerHandler(backend, controllerHandlerFactory, controllerID) {}
    ~Axi4_Intc() override = default;

    void handle() override;

    static std::unique_ptr<Axi4_Intc> create(DeviceBackend*, InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, std::string desrciption);
  };

} // namespace ChimeraTK
