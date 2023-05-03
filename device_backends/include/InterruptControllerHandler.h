// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"
#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {
  class InterruptControllerHandler;

  /** Knows which type of InterruptControllerHandler to create for which interrupt.
   *  It is filled from the meta information from the map file.
   */
  class InterruptControllerHandlerFactory {
   public:
    InterruptControllerHandlerFactory(NumericAddressedBackend* backend);

    std::unique_ptr<InterruptControllerHandler> createInterruptControllerHandler(std::vector<uint32_t> controllerID);
    void addInterruptController(std::vector<uint32_t> controllerID, std::string name, std::string description);

   protected:
    NumericAddressedBackend* _backend;

    /** The key of this map is the controllerID.
     *  The value is a string pair of controller name and the description string from the map file.
     */
    std::map<std::vector<uint32_t>, std::pair<std::string, std::string>> _controllerDescriptions;

    /** Each controller type is registered via name and creator function.
     */
    std::map<std::string,
        std::function<std::unique_ptr<InterruptControllerHandler>(NumericAddressedBackend*, std::string)>>
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
    explicit InterruptControllerHandler(NumericAddressedBackend* backend) : _backend(backend) {}
    virtual ~InterruptControllerHandler() = default;

    /** Called by the NumericAddressedBackend during instantiation.
     */
    void activateInterrupt(uint32_t);
    /** The NumericAddressedBackends needs to access the InterruptDispatchers to place nested InterruptController
     * during initialisation
     */
    boost::shared_ptr<NumericAddressedInterruptDispatcher> const& getInterruptDispatcher(uint32_t);

    /** The interrupt handling functions implements the handshake with the interrupt controller. It needs to
     * beimplemented individually for each interrupt controller.
     */
    virtual void handle() = 0;

   protected:
    /** Each known interrupt has its own dispatcher
     */
    std::map<uint32_t, boost::shared_ptr<NumericAddressedInterruptDispatcher>> _dispatchers;

    NumericAddressedBackend* _backend;
  };

} // namespace ChimeraTK
