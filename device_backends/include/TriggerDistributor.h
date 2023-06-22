// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "VersionNumber.h"

#include <boost/make_shared.hpp>

#include <mutex>

namespace ChimeraTK {
  class InterruptControllerHandlerFactory;
  class InterruptControllerHandler;
  class TriggeredPollDistributor;

  template<typename UserType>
  class VariableDistributor;

  /** Distribute a void type interrupt signal (trigger) to three possible consumers:
   *  \li InterruptControllerHandler
   *  \li TriggeredPollDistributor
   *  \li VariableDistributor<void>
   */
  class TriggerDistributor : public boost::enable_shared_from_this<TriggerDistributor> {
   public:
    void trigger(VersionNumber v);

    TriggerDistributor(DeviceBackend* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> interruptID, boost::shared_ptr<InterruptControllerHandler> parent);

    void activate(VersionNumber v);
    void deactivate();
    void sendException(const std::exception_ptr& e);

    boost::shared_ptr<TriggeredPollDistributor> getPollDistributorRecursive(std::vector<uint32_t> const& interruptID);
    boost::shared_ptr<VariableDistributor<ChimeraTK::Void>> getVariableDistributorRecursive(
        std::vector<uint32_t> const& interruptID);

   protected:
    std::recursive_mutex _creationMutex;
    std::vector<uint32_t> _id;
    DeviceBackend* _backend;
    InterruptControllerHandlerFactory* _interruptControllerHandlerFactory;
    boost::weak_ptr<InterruptControllerHandler> _interruptControllerHandler;
    boost::weak_ptr<TriggeredPollDistributor> _pollDistributor;
    boost::weak_ptr<VariableDistributor<ChimeraTK::Void>> _variableDistributor;
    boost::shared_ptr<InterruptControllerHandler> _parent;
    bool _isActive{false};
  };

} // namespace ChimeraTK
