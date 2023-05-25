// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/VersionNumber.h>

#include <boost/shared_ptr.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace ChimeraTK {
  class InterruptControllerHandler;
  class TriggeredPollDistributor;
  class DeviceBackend;

  /** Knows which type of InterruptControllerHandler to create for which interrupt.
   *  It is filled from the meta information from the map file.
   */
  class InterruptControllerHandlerFactory {
   public:
    explicit InterruptControllerHandlerFactory(DeviceBackend* backend);

    std::unique_ptr<InterruptControllerHandler> createInterruptControllerHandler(
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggeredPollDistributor> parent);
    void addControllerDescription(
        std::vector<uint32_t> const& controllerID, std::string const& name, std::string const& description);

   protected:
    DeviceBackend* _backend;

    /** The key of this map is the controllerID.
     *  The value is a string pair of controller name and the description string from the map file.
     */
    std::map<std::vector<uint32_t>, std::pair<std::string, std::string>> _controllerDescriptions;

    /** Each controller type is registered via name and creator function.
     */
    std::map<std::string,
        std::function<std::unique_ptr<InterruptControllerHandler>(DeviceBackend*, InterruptControllerHandlerFactory*,
            std::vector<uint32_t> const&, std::string, boost::shared_ptr<TriggeredPollDistributor>)>>
        _creatorFunctions;
  };

  /** Interface base class for interrupt controller handlers. It implements the interface with the
   * DeviceBackend and the InterruptDispatchers. Implementations must fill the pure virtual "handle()"
   * function with life and register the constructor to the factory.
   */
  class InterruptControllerHandler : public std::enable_shared_from_this<InterruptControllerHandler> {
   public:
    /** InterruptControllerHandler classes must only be constructed inside and held by a DeviceBackend,
     * which is known to the handler via plain pointer (to avoid shared pointer loops)
     */
    InterruptControllerHandler(DeviceBackend* backend, InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggeredPollDistributor> parent)
    : _backend(backend), _controllerHandlerFactory(controllerHandlerFactory), _id(controllerID), _parent(parent) {}
    virtual ~InterruptControllerHandler() = default;

    /** Needed to get a new accessor for a certain interrupt. The whole chain will be created recursively if it does not
     * exist yet.
     */
    [[nodiscard]] boost::shared_ptr<TriggeredPollDistributor> getTriggerPollDistributorRecursive(
        std::vector<uint32_t> const& interruptID, bool activateIfNew);

    void activate(VersionNumber version);
    void sendException(const std::exception_ptr& e);
    void deactivate();

    /** The interrupt handling functions implements the handshake with the interrupt controller. It needs to
     * be implemented individually for each interrupt controller.
     */
    virtual void handle(VersionNumber version) = 0;

   protected:
    /** Each known interrupt has its own dispatcher
     */
    std::map<uint32_t, boost::weak_ptr<TriggeredPollDistributor>> _dispatchers;

    DeviceBackend* _backend;
    InterruptControllerHandlerFactory* _controllerHandlerFactory;

    /** The ID of this controller handler.
     */
    std::vector<uint32_t> _id;

    boost::shared_ptr<TriggeredPollDistributor> _parent;
  };

} // namespace ChimeraTK
