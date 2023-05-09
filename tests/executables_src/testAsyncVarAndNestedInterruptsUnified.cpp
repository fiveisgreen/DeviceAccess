// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE AsyncVarAndNestedInterruptsUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
//#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
//#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

/* ===============================================================================================
 * This test is checking async variables and the map-file related part of interrupts for
 * consistency with the specification (implemented in the unified test).
 * - AsyncNDRegisterAccessor
 * - AsyncVariable (multiple listeners to one logical async variable)
 * - Basic interrupt controller handler functionality (via DummyInterruptControllerHandler)
 * - TriggeredPollDistributor (FIXME: currently mixed with AsyncVariable in AsyncAccessorManager and
 *                                    NumericAddressedInterruptDispatcher)
 * - Instaniation from the map file
 *
 * FIXME: Unified test does not support void variables yet.
 * ==============================================================================================*/

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(AsyncVarAndNestedInterruptsUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=testNestedInterrupts.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

template<class WITHPATH, uint32_t INTERRUPT>
struct TriggeredInt {
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", WITHPATH::path()};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{acc + INTERRUPT}}; // just re-use the interrupt here. Any number does the job.
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{acc}};
  }

  void setRemoteValue() {
    acc = generateValue<minimumUserType>()[0][0];
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }

  void forceAsyncReadInconsistency() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }
};

struct datafrom6 : public TriggeredInt<datafrom6, 6> {
  static std::string path() { return "/datafrom6"; }
};

struct datafrom5_9 : public TriggeredInt<datafrom5_9, 5> {
  static std::string path() { return "/datafrom5_9"; }
};

struct datafrom4_8_2 : public TriggeredInt<datafrom4_8_2, 4> {
  static std::string path() { return "/datafrom4_8_2"; }
};

struct datafrom4_8_3 : public TriggeredInt<datafrom4_8_3, 4> {
  static std::string path() { return "/datafrom4_8_3"; }
};

/**********************************************************************************************************************/


BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<datafrom6>()
      .addRegister<datafrom5_9>()
      .addRegister<datafrom4_8_2>()
      .addRegister<datafrom4_8_3>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
