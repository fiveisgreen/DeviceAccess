// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/shared_ptr.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace ChimeraTK {
  class InterruptControllerHandler;
  class NumericAddressedInterruptDispatcher;
  class NumericAddressedBackend;

  /** Knows which type of InterruptControllerHandler to create for which interrupt.
   *  It is filled from the meta information from the map file.
   */
  class InterruptControllerHandlerFactory {
   public:
    explicit InterruptControllerHandlerFactory(NumericAddressedBackend* backend);

    std::unique_ptr<InterruptControllerHandler> createInterruptControllerHandler(
        std::vector<uint32_t> const& controllerID);
    void addInterruptController(
        std::vector<uint32_t> const& controllerID, std::string const& name, std::string const& description);

   protected:
    NumericAddressedBackend* _backend;

    /** The key of this map is the controllerID.
     *  The value is a string pair of controller name and the description string from the map file.
     */
    std::map<std::vector<uint32_t>, std::pair<std::string, std::string>> _controllerDescriptions;

    /** Each controller type is registered via name and creator function.
     */
    std::map<std::string,
        std::function<std::unique_ptr<InterruptControllerHandler>(
            NumericAddressedBackend*, std::vector<uint32_t> const&, std::string)>>
        _creatorFunctions;
  };

  /** Interface base class for interrupt controller handlers. It implements the interface with the
   * NumericAddressedBackend and the InterruptDispatchers. Implementations must fill the pure virtual "handle()"
   * function with life and register the constructor to the factory.
   */
  class InterruptControllerHandler {
   public:
    /** InterruptControllerHandler classes must only be constructed inside and held by a NumericAddressedBackend,
     * which is known to the handler via plain pointer (to avoid shared pointer loops)
     */
    explicit InterruptControllerHandler(NumericAddressedBackend* backend, std::vector<uint32_t> const& controllerID)
    : _backend(backend), _id(controllerID) {}
    virtual ~InterruptControllerHandler() = default;

    /** Called by the NumericAddressedBackend during instantiation.
     */
    void addInterrupt(std::vector<uint32_t> const& interruptID);
    /** The NumericAddressedBackends needs to access the InterruptDispatchers to place nested InterruptController
     * during initialisation
     */
    [[nodiscard]] boost::shared_ptr<NumericAddressedInterruptDispatcher> const& getInterruptDispatcher(
        uint32_t interruptNumber) const;

    /** The interrupt handling functions implements the handshake with the interrupt controller. It needs to
     * beimplemented individually for each interrupt controller.
     */
    virtual void handle() = 0;

   protected:
    /** Each known interrupt has its own dispatcher
     */
    std::map<uint32_t, boost::shared_ptr<NumericAddressedInterruptDispatcher>> _dispatchers;

    NumericAddressedBackend* _backend;

    /** The ID of this controller handler.
     */
    std::vector<uint32_t> _id;
  };

} // namespace ChimeraTK
