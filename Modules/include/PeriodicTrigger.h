#ifndef CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H
#define CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H

#include "ApplicationCore.h"

#include <chrono>

namespace ChimeraTK {

  /**
   * Simple periodic trigger that fires a variable once per second.
   * After configurable number of seconds it will wrap around
   */
  struct PeriodicTrigger : public ApplicationModule {

      /** Constructor. In addition to the usual arguments of an ApplicationModule, the default timeout value is specified.
       *  This value is used as a timeout if the timeout value is set to 0. The timeout value is in milliseconds. */
      PeriodicTrigger(EntityOwner *owner, const std::string &name, const std::string &description,
             const uint32_t defaultTimeout=1000, bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={})
      : ApplicationModule(owner, name, description, eliminateHierarchy, tags),
        defaultTimeout_(defaultTimeout)
      {}

      ScalarPollInput<uint32_t> timeout{this, "timeout", "ms", "Timeout in milliseconds. The trigger is sent once per the specified duration."};
      ScalarOutput<uint64_t> tick{this, "tick", "", "Timer tick. Counts the trigger number starting from 0."};

      void mainLoop() {
        tick = 0;
        if(timeout = 0) timeout = defaultTimeout_;

        std::chrono::time_point<std::chrono::steady_clock> t = std::chrono::steady_clock::now();

        while(true) {
          timeout.read();
          t += static_cast<uint32_t>(timeout) * std::chrono::milliseconds();
          std::this_thread::sleep_until(t);

          tick++;
          tick.write();
        }
      }

    private:

      uint32_t defaultTimeout_;

  };
}

#endif // CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H
