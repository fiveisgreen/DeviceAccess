// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackendImpl.h"
#include "VersionNumber.h"

#include <boost/make_shared.hpp>

#include <mutex>

namespace ChimeraTK {
  class InterruptControllerHandlerFactory;
  class InterruptControllerHandler;
  class TriggeredPollDistributor;
  class AsyncDomain;

  template<typename UserType>
  class VariableDistributor;

  /** Distribute a void type interrupt signal (trigger) to three possible consumers:
   *  \li InterruptControllerHandler
   *  \li TriggeredPollDistributor
   *  \li VariableDistributor<nullptr_t>
   */
  class TriggerDistributor : public boost::enable_shared_from_this<TriggerDistributor> {
   public:
    void distribute(std::nullptr_t, VersionNumber v);

    TriggerDistributor(DeviceBackendImpl* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> interruptID, boost::shared_ptr<InterruptControllerHandler> parent,
        boost::shared_ptr<AsyncDomain> asyncDomain);

    void activate(std::nullptr_t, VersionNumber v);
    void sendException(const std::exception_ptr& e);

    boost::shared_ptr<TriggeredPollDistributor> getPollDistributorRecursive(std::vector<uint32_t> const& interruptID);
    boost::shared_ptr<VariableDistributor<std::nullptr_t>> getVariableDistributorRecursive(
        std::vector<uint32_t> const& interruptID);

    /**
     * Common implementation for getPollDistributorRecursive and getVariableDistributorRecursive to avoid code duplication.
     */
    /* Don't remove the non-templated functions when refactoring. They are needed to instantiate the template code. We
     * cannot put the template code into the header because it needs full class descriptions, which would lead to
     * circular header inclusion.
     */
    template<typename DistributorType>
    boost::shared_ptr<DistributorType> getDistributorRecursive(std::vector<uint32_t> const& interruptID);

    boost::shared_ptr<AsyncDomain> getAsyncDomain() { return _asyncDomain; }

   protected:
    std::recursive_mutex _creationMutex;
    std::vector<uint32_t> _id;
    // We have to use a plain pointer here because the primary trigger distributors are already created in the
    // constructor, where the shared_ptr is not available yet. It is just set there and only used later.
    DeviceBackendImpl* _backend;
    InterruptControllerHandlerFactory* _interruptControllerHandlerFactory;
    boost::weak_ptr<InterruptControllerHandler> _interruptControllerHandler;
    boost::weak_ptr<TriggeredPollDistributor> _pollDistributor;
    boost::weak_ptr<VariableDistributor<std::nullptr_t>> _variableDistributor;
    boost::shared_ptr<InterruptControllerHandler> _parent;
    boost::shared_ptr<AsyncDomain> _asyncDomain;
  };

} // namespace ChimeraTK
