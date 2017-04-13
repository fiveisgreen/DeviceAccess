/*
 * Application.h
 *
 *  Created on: Jun 07, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_H
#define CHIMERATK_APPLICATION_H

#include <mutex>
#include <atomic>

#include <mtca4u/DeviceBackend.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>

#include "ApplicationException.h"
#include "VariableNetwork.h"
#include "Flags.h"
#include "InternalModule.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  class Module;
  class AccessorBase;
  class VariableNetwork;
  class TriggerFanOut;

  template<typename UserType>
  class Accessor;

  template<typename UserType>
  class TestDecoratorRegisterAccessor;

  class Application : public ApplicationBase, public EntityOwner {

    public:

      Application(const std::string& name)
      : ApplicationBase(name), EntityOwner(nullptr, name, "") {}

      ~Application() {}

      /** This will remove the global pointer to the instance and allows creating another instance
       *  afterwards. This is mostly useful for writing tests, as it allows to run several applications sequentially
       *  in the same executable. Note that any ApplicationModules etc. owned by this Application are no longer
       *  valid after destroying the Application and must be destroyed as well (or at least no longer used). */
      void shutdown();
      
      /** Define the connections between process variables. Must be implemented by the application developer. */
      virtual void defineConnections() = 0;
      
      void initialise();

      void run();
      
      /** Check if all connections are valid. */
      void checkConnections();

      /** Instead of running the application, just initialise it and output the published variables to an XML file. */
      void generateXML();

      /** Output the connections requested in the initialise() function to std::cout. This may be done also before
       *  makeConnections() has been called. */
      void dumpConnections();
      
      /** Enable warning about unconnected variables. This can be helpful to identify missing connections but is
       *  disabled by default since it may often be very noisy. */
      void warnUnconnectedVariables() { enableUnconnectedVariablesWarning = true; }

      /** Obtain instance of the application. Will throw an exception if called before the instance has been
       *  created by the control system adapter, or if the instance is not based on the Application class. */
      static Application& getInstance();
      
      /** Enable the testable mode. This allows to pause and resume the application for testing purposes using the
       *  functions pauseApplication() and resumeApplication(). The application will start in paused state.
       * 
       *  This function must be called before the application is initialised (i.e. before the call to initialise()).
       * 
       *  Note: Enabling the testable mode will have a singificant impact on the performance, since it will prevent
       *  any module threads to run at the same time! */
      void enableTestableMode() {
        testableMode = true;
        testableModeLock("enableTestableMode");
        testableModeThreadName() = "TEST THREAD";
      }

      /** Resume the application until all application threads are stuck in a blocking read operation. Works only when
       *  the testable mode was enabled. */
      void stepApplication();
      
      /** Enable some additional (potentially noisy) debug output for the testable mode. Can be useful if tests
       *  of applications seem to hang for no reason in stepApplication. */
      void debugTestableMode() { enableDebugTestableMode = true; }
      
      /** This is a testable version of mtca4u::TransferElement::readAny(). Always use this version instead of the
       *  original version provided by DeviceAccess. If the testable mode is not enabled, just the original version
       *  is called instead. Only with the testable mode enabled, special precautions are taken to make this blocking
       *  call testable. */
      static boost::shared_ptr<TransferElement> readAny(std::list<std::reference_wrapper<TransferElement>> elementsToRead);
      
      /** Lock the testable mode mutex for the current thread. Internally, a thread-local std::unique_lock<std::mutex>
       *  will be created and re-used in subsequent calls within the same thread to this function and to
       *  testableModeUnlock().
       *
       *  This function should generally not be used in user code. */
      static void testableModeLock(const std::string& name) {
        if(!getInstance().testableMode) return;
        if(getInstance().enableDebugTestableMode && !getInstance().testableMode_repeatingMutexOwner) {
          std::cout << "Application::testableModeLock(): Thread " << testableModeThreadName()
                    << " tries to obtain lock for " << name << std::endl;
        }
        getTestableModeLockObject().lock();
        if(getInstance().testableMode_lastMutexOwner == std::this_thread::get_id()) {
          if(!getInstance().testableMode_repeatingMutexOwner) {
            getInstance().testableMode_repeatingMutexOwner = true;
            std::cout << "Application::testableModeLock(): Thread " << testableModeThreadName()
                      << " repeatedly obtained lock successfully for " << name << ". Further messages will be suppressed." << std::endl;
          }
        }
        else {
          getInstance().testableMode_repeatingMutexOwner = false;
        }
        getInstance().testableMode_lastMutexOwner = std::this_thread::get_id();
        if(getInstance().enableDebugTestableMode && !getInstance().testableMode_repeatingMutexOwner) {
          std::cout << "Application::testableModeLock(): Thread " << testableModeThreadName()
                    << " obtained lock successfully for " << name << std::endl;
        }
      }
      
      /** Unlock the testable mode mutex for the current thread. See also testableModeLock().
       * 
       *  Initially the lock will not be owned by the current thread, so the first call to this function will throw an
       *  exception (see std::unique_lock::unlock()), unless testableModeLock() has been called first.
       *
       *  This function should generally not be used in user code. */
      static void testableModeUnlock(const std::string& name) {
        if(!getInstance().testableMode) return;
        if(getInstance().enableDebugTestableMode && (!getInstance().testableMode_repeatingMutexOwner
          || getInstance().testableMode_lastMutexOwner != std::this_thread::get_id())) {
          std::cout << "Application::testableModeUnlock(): Thread " << testableModeThreadName()
                    << " releases lock for " << name << std::endl;
        }
        getTestableModeLockObject().unlock();
      }
      
      /** Test if the testable mode mutex is locked by the current thread.
       *
       *  This function should generally not be used in user code. */
      static bool testableModeTestLock() {
        if(!getInstance().testableMode) return false;
        return getTestableModeLockObject().owns_lock();
      }

      /** Get string holding the name of the current thread for debugging output of the testable mode. */
      static std::string& testableModeThreadName() {
        thread_local static std::string name{"**UNNAMED**"};
        return name;
      }

    protected:

      friend class Module;
      friend class VariableNetwork;
      friend class VariableNetworkNode;

      template<typename UserType>
      friend class Accessor;

      /** Obtain the lock object for the testable mode lock for the current thread. The returned object has
       *  thread_local storage duration and must only be used inside the current thread. Initially (i.e. after
       *  the first call in one particular thread) the lock will not be owned by the returned object, so it is
       *  important to catch the corresponding exception when calling std::unique_lock::unlock(). */
      static std::unique_lock<std::mutex>& getTestableModeLockObject() {
        thread_local static std::unique_lock<std::mutex> myLock(Application::testableMode_mutex, std::defer_lock);
        return myLock;
      }
      
      /** Register the connections to constants for previously unconnected nodes. */
      void processUnconnectedNodes();

      /** Make the connections between accessors as requested in the initialise() function. */
      void makeConnections();

      /** Make the connections for a single network */
      void makeConnectionsForNetwork(VariableNetwork &network);

      /** UserType-dependent part of makeConnectionsForNetwork() */
      template<typename UserType>
      void typedMakeConnection(VariableNetwork &network);

      /** Register a connection between two VariableNetworkNode */
      VariableNetwork& connect(VariableNetworkNode a, VariableNetworkNode b);

      /** Perform the actual connection of an accessor to a device register */
      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createDeviceVariable(const std::string &deviceAlias,
          const std::string &registerName, VariableDirection direction, UpdateMode mode, size_t nElements);

      /** Create a process variable with the PVManager, which is exported to the control system adapter. nElements will
          be the array size of the created variable. */
      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createProcessVariable(VariableNetworkNode const &node);

      /** Create a local process variable which is not exported. The first element in the returned pair will be the
       *  sender, the second the receiver. */
      template<typename UserType>
      std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> >
            createApplicationVariable(VariableNetworkNode const &node);

      /** List of InternalModules */
      std::list<boost::shared_ptr<InternalModule>> internalModuleList;

      /** List of variable networks */
      std::list<VariableNetwork> networkList;

      /** List of constant variable nodes */
      std::list<VariableNetworkNode> constantList;
      
      /** Map of trigger sources to their corresponding TriggerFanOuts. Note: the key is the unique ID of the
       *  triggering node. */
      std::map<const void*, boost::shared_ptr<TriggerFanOut>> triggerMap;

      /** Create a new, empty network */
      VariableNetwork& createNetwork();

      /** Instance of VariableNetwork to indicate an invalid network */
      VariableNetwork invalidNetwork;

      /** Map of DeviceBackends used by this application. The map key is the alias name from the DMAP file */
      std::map<std::string, boost::shared_ptr<mtca4u::DeviceBackend>> deviceMap;
      
      /** Flag if connections should be made in testable mode (i.e. the TestDecoratorRegisterAccessor is put around all
       *  push-type input accessors etc.). */
      bool testableMode{false};
      
      /** Mutex used in testable mode to take control over the application threads. Use only through the lock object 
       *  obtained through getLockObjectForCurrentThread().
       *
       *  This member is static, since it should survive destroying an application instance and creating a new one.
       *  Otherwise getTestableModeLockObject() would not work, since it relies on thread_local instances which have to
       *  be static. The static storage duration presents no problem in either case, since there can only be one single
       *  instance of Application at a time (see ApplicationBase constructor). */
      static std::mutex testableMode_mutex;
      
      /** Semaphore counter used in testable mode to check if application code is finished executing. This value may
       *  only be accessed while holding the testableMode_mutex. */
      size_t testableMode_counter{0};
      
      /** Flag if noisy debug output is enabled for the testable mode */
      bool enableDebugTestableMode{false};
      
      /** Flag whether to warn about unconnected variables or not */
      bool enableUnconnectedVariablesWarning{false};
      
      /** Last thread which successfully obtained the lock for the testable mode. This is used to prevent spamming
       *  repeating messages if the same thread acquires and releases the lock in a loop without another thread
       *  activating in between. */
      std::thread::id testableMode_lastMutexOwner;
      
      /** @todo DOCU */
      std::atomic<bool> testableMode_repeatingMutexOwner{false};

      template<typename UserType>
      friend class TestDecoratorRegisterAccessor;   // needs access to the testableMode_mutex and testableMode_counter

      template<typename UserType>
      friend class TestDecoratorTransferFuture;   // needs access to the testableMode_mutex and testableMode_counter

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
