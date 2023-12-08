// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncNDRegisterAccessor.h"

#include <mutex>
#include <utility>

namespace ChimeraTK {

  /** Typeless base class. The implementations will have a weak pointer of an AsyncNDRegisterAccessor<UserType>
   *  and will implement the pure virtual functions which act on the accessor.
   */
  struct AsyncVariable {
    virtual ~AsyncVariable() = default;

    /** Send the value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     *  The buffer is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.
     */
    virtual void send() = 0;

    /** Send an exception to all subscribers. Must only be called while holding the exclusive isAsyncActive lock.
     */
    virtual void sendException(std::exception_ptr e) = 0;

    /** Helper functions for the creation of an AsyncNDRegisterAccessor. As the creating code cannot use
     *  the catalogue, each backend has to implement these functions appropriately.
     */
    virtual unsigned int getNumberOfChannels() = 0;
    virtual unsigned int getNumberOfSamples() = 0;
    virtual const std::string& getUnit() = 0;
    virtual const std::string& getDescription() = 0;
    virtual bool isWriteable() = 0;

    /** Fill the user buffer from the sync accessor, and replace the version number with the given version.
     */
    virtual void fillSendBuffer(VersionNumber const& version) = 0;
  };

  /** Helper class to have a complete descriton to create an Accessor.
   *  It contains all the information given to DeviceBackend:getNDRegisterAccessor, incl. the offset in the
   *  register which is not known to the catalogue entry and the accessor itself. Just just to keep the number of
   * parameters for createAsyncVariable in check.
   */
  struct AccessorInstanceDescriptor {
    RegisterPath name;
    std::type_index type;
    size_t numberOfWords;
    size_t wordOffsetInRegister;
    AccessModeFlags flags;
    AccessorInstanceDescriptor(const RegisterPath& name_, std::type_index type_, size_t numberOfWords_,
        size_t wordOffsetInRegister_, AccessModeFlags flags_)
    : name(name_), type(type_), numberOfWords(numberOfWords_), wordOffsetInRegister(wordOffsetInRegister_),
      flags(std::move(flags_)) {}
  };

  /** The AsyncAccessorManager has three main functionalities:
   *  * It manages the subscription/unsubscription mechanism.
   *  * It serves as a factory for the asynchronous accessors which are during
   *    the subscription to get consistent behaviour.
   *  * The manager provides functions for all asynchronous accessors subscribed
   *    to this manager, like enabling, disabling or sending exceptions.
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class implements a lock so data
   *  distribution to the variables is safe against concurrent subscriptions/unsubscriptions.
   *
   *  The AsyncAccessorManager has some pure virtual functions. They have implementation
   *  specific functionality which must be provided by a derived version of the AsyncAccessorManager.
   */
  class AsyncAccessorManager : public boost::enable_shared_from_this<AsyncAccessorManager> {
   public:
    explicit AsyncAccessorManager(boost::shared_ptr<DeviceBackendImpl> backend) : _backend(backend) {}
    virtual ~AsyncAccessorManager() = default;

    /** Request a new subscription. This function internally creates the correct asynchronous accessor and a matching
     * AsyncVariable. A weak pointer to the AsyncNDRegisterAccessor is registered in the AsyncVariable, and a shared
     * pointer is returned to the calling code.
     */
    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /** This function must only be called from the destructor of the AsyncNDRegisterAccessor which is created in the
     * subscribe function!
     */
    void unsubscribe(TransferElementID id);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(const std::exception_ptr& e);

    /** Activate all accessors and send the initial value. Generates a new version number which is used for
     *  all initial values and  which can be read out with getLastVersion().
     *  This function has to be provided by each AsyncAccessorManager implementation.
     */
    virtual void activate(VersionNumber version) = 0;

   protected:
    /*** Each implementation must provide a function to create specific AsyncVariables.
     *   When the variable is returned, async accessor is not set yet. This would leave the whole creation
     *   of the async accessor to the backend, would mean a lot of code duplication. It also cannot be
     *   retrieved from the AsyncVariable as this only contains a weak pointer.
     *   If the isActive flag is set, the _sendBuffer must already contain the initial value. The variable itself
     *   is not activated yet as the async accessor is still not set.
     */
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(
        createAsyncVariable, std::unique_ptr<AsyncVariable>(AccessorInstanceDescriptor const&));

    // This mutex protects the _asyncVariables container and all its contents. You must not
    // touch those variables without holding the mutex. It serves two purposes:
    // 1. Variables can be added or removed from the container at any time. It is not safe to handle an element without
    // holding the lock.
    // 2. The elements in the container are not thread-safe as well. We use the same lock as it is needed for 1. anyway.
    std::recursive_mutex _variablesMutex;
    std::map<TransferElementID, std::unique_ptr<AsyncVariable>> _asyncVariables; ///< protected by _variablesMutex

    boost::shared_ptr<DeviceBackendImpl> _backend;

    /// this virtual function lets derived classes react on subscribe / unsubscribe
    /// _variablesMutex locked during call
    virtual void asyncVariableMapChanged() {}

  };

  /** AsyncVariableImpl contains a weak pointer to an AsyncNDRegisterAccessor<UserType> and a send buffer
   * NDRegisterAccessor<UserType>::Buffer. This class provides implementation for those functions of the AsyncVariable
   * which touch the AsyncNDRegisterAccessor. It does implement the functions which provide the information needed to
   * create an AsyncNDRegisterAccessor, like getNumberOfChannels(). These are backend specific and need a dedicated
   * implementation per backend.
   *
   *  The AsyncVariableManager cannot hold a shared pointer of the AsyncNDRegisterAccessor because then you could never
   * get rid of a created accessor, which means the manager would just keep growing in memory if accessors are
   * dynamically created and removed. Hence a weak pointer is used, and this class provides all the functions that
   * access this weak pointer and do all the locking and nullptr checking.
   */
  template<typename UserType>
  struct AsyncVariableImpl : public AsyncVariable {
    AsyncVariableImpl(size_t nChannels, size_t nElements);

    void send() final;
    void sendException(std::exception_ptr e) final;

    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;

   private:
    // This weak pointer is private so the user cannot bypass correct locking and nullptr-checking.
    boost::weak_ptr<AsyncNDRegisterAccessor<UserType>> _asyncAccessor;
    friend AsyncAccessorManager; // is allowed to set the _asyncAccessor
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->sendException(e);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  AsyncVariableImpl<UserType>::AsyncVariableImpl(size_t nChannels, size_t nElements)
  : _sendBuffer(nChannels, nElements) {}

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::send() {
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->sendDestructively(_sendBuffer);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> AsyncAccessorManager::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    auto untypedAsyncVariable = CALL_VIRTUAL_FUNCTION_TEMPLATE(createAsyncVariable, UserType, descriptor);

    auto asyncVariable = dynamic_cast<AsyncVariableImpl<UserType>*>(untypedAsyncVariable.get());
    // we take all the information we need for the async accessor from AsyncVariable because we cannot use the catalogue
    // here
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType>>(_backend, shared_from_this(), name,
        asyncVariable->getNumberOfChannels(), asyncVariable->getNumberOfSamples(), flags, asyncVariable->getUnit(),
        asyncVariable->getDescription());
    // Set the exception backend here. It might be that the accessor is already activated during subscription, and the
    // backend should be set at that point
    newSubscriber->setExceptionBackend(_backend);

    if(asyncVariable->isWriteable()) {
      // for writeable variables we add another synchronous accessors (which knows how to access the data) to the
      // asyncAccessor (which is generic)
      auto synchronousFlags = flags;
      synchronousFlags.remove(AccessMode::wait_for_new_data);
    }

    asyncVariable->_asyncAccessor = newSubscriber;
    // Now that the AsyncVariable is complete we can finally activate it.
    if(_backend->isAsyncReadActive()) {
      asyncVariable->fillSendBuffer({});
      asyncVariable->send();
    }

    { // scope for the lock guard
      std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
      _asyncVariables[newSubscriber->getId()] = std::move(untypedAsyncVariable);
    }

    asyncVariableMapChanged();
    return newSubscriber;
  }

} // namespace ChimeraTK
