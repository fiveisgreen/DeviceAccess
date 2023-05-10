// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  class DummyIntc : public InterruptControllerHandler {
   public:
    explicit DummyIntc(NumericAddressedBackend* backend, std::vector<uint32_t> const& controllerID)
    : InterruptControllerHandler(backend, controllerID) {}
    ~DummyIntc() override = default;

    void handle() override;

    static std::unique_ptr<DummyIntc> create(
        NumericAddressedBackend*, std::vector<uint32_t> const& controllerID, std::string const& desrciption);
  };

} // namespace ChimeraTK
