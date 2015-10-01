#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "BackendFactory.h"
#include "DummyBackend.h"
#include "NotImplementedException.h"
using namespace mtca4u;

#define TEST_MAPPING_FILE "mtcadummy_withoutModules.map"
#define FIRMWARE_REGISTER_STRING "WORD_FIRMWARE"
#define STATUS_REGISTER_STRING "WORD_STATUS"
#define USER_REGISTER_STRING "WORD_USER"
#define CLOCK_MUX_REGISTER_STRING "WORD_CLK_MUX"
#define CLOCK_RESET_REGISTER_STRING "WORD_CLK_RST"
#define EXISTING_DEVICE "DUMMYD0"
#define NON_EXISTING_DEVICE "DUMMY9"

static BackendFactory FactoryInstance = BackendFactory::getInstance();
// declaration so we can make it friend
// of TestableDummyBackend.
class DummyBackendTest;

/** The TestableDummybackend is derived from
 *  DummyBackend to get access to the protected members.
 *  This is done by declaring DummybackendTest as a friend.
 */
class TestableDummyBackend : public DummyBackend {
public:
	TestableDummyBackend(std::string mapFileName)
: DummyBackend(mapFileName) {}
	friend class DummyBackendTest;
};

class DummyBackendTest {
public:
  DummyBackendTest()
: a(0), b(0), c(0), _backendInstance()
{
    std::list<std::string> parameters;
    parameters.push_back(std::string(TEST_MAPPING_FILE));
    _dummyBackend = boost::shared_ptr<TestableDummyBackend>(new TestableDummyBackend(TEST_MAPPING_FILE) );
}

  static void testCalculateVirtualAddress();
  static void testCheckSizeIsMultipleOfWordSize();
  static void testAddressRange();
  // void testOpenClose();
  void testReadWriteSingleWordRegister();
  /// The read function can be readDMA or readArea, the writeFunction is always
  /// writeArea.
  /// WriteDMA probably does not make sense at all.
  /// The argument is a pointer to a member function of the DummyBackend class,
  /// which
  /// returns void and takes uint32_t, int32_t*, size_t, uint8_t as arguments.
  void testReadWriteMultiWordRegister(
      //void (DummyBackend::*readFunction)(uint32_t, int32_t*, size_t, uint8_t));
      void (DummyBackend::*readFunction)(uint8_t, uint32_t, int32_t*, size_t));
  void testWriteDMA();
  void testReadDeviceInfo();
  void testReadOnly();
  void testWriteCallbackFunctions();
  void testIsWriteRangeOverlap();
  void testWriteRegisterWithoutCallback();
  /// Test that all registers, read-only flags and callback functions are
  /// removed
  void testFinalClosing();

  // Try Creating a backend and check if it is connected.
  void testCreateBackend();

  // Try opening the created backend and check it's open status.
  void testOpen();

  // Try closing the created backend and check it's open status.
  void testClose();

  void testOpenClose();

private:
  boost::shared_ptr<TestableDummyBackend> _dummyBackend;
  void freshlyopenice();
  TestableDummyBackend* getBackendInstance(bool reOpen = false);
  friend class DummyBackendTestSuite;

  // stuff for the callback function test
  int a, b, c;
  void increaseA() { ++a; }
  void increaseB() { ++b; }
  void increaseC() { ++c; }
  boost::shared_ptr<mtca4u::DeviceBackend> _backendInstance;
};

class DummyBackendTestSuite : public test_suite {
public:
	DummyBackendTestSuite() : test_suite("DummyBackend test suite") {
		boost::shared_ptr<DummyBackendTest> dummyBackendTest(new DummyBackendTest);

		// Pointers to test cases with dependencies. All other test cases are added
		// directly.
		test_case* readOnlyTestCase =
				BOOST_CLASS_TEST_CASE(&DummyBackendTest::testReadOnly, dummyBackendTest);
		test_case* writeCallbackFunctionsTestCase = BOOST_CLASS_TEST_CASE(
				&DummyBackendTest::testWriteCallbackFunctions, dummyBackendTest);
		test_case* writeRegisterWithoutCallbackTestCase = BOOST_CLASS_TEST_CASE(
				&DummyBackendTest::testWriteRegisterWithoutCallback, dummyBackendTest);

		test_case* createBackendTestCase = BOOST_CLASS_TEST_CASE(
				&DummyBackendTest::testCreateBackend, dummyBackendTest);
		test_case* openTestCase = BOOST_CLASS_TEST_CASE(
				&DummyBackendTest::testOpen, dummyBackendTest);
		test_case* openCloseTestCase = BOOST_CLASS_TEST_CASE(
				&DummyBackendTest::testOpenClose, dummyBackendTest);
		test_case* closeTestCase = BOOST_CLASS_TEST_CASE(
		   &DummyBackendTest::testClose, dummyBackendTest);

		// we use the setup from the read-only test to check that the callback
		// function is not executed if the register is not writeable.
		writeCallbackFunctionsTestCase->depends_on(readOnlyTestCase);
		writeRegisterWithoutCallbackTestCase->depends_on(
				writeCallbackFunctionsTestCase);

		// As boost test cases need nullary functions (working with parameters would
		// not work with function pointers)
		// we create them by binding readArea and readDMA to the
		// testReadWriteMultiWordRegister function of the
		// dummyBackendTest instance.
		boost::function<void(void)> testReadWriteArea =
				boost::bind(&DummyBackendTest::testReadWriteMultiWordRegister,
						dummyBackendTest, &DummyBackend::read);

		/*boost::function<void(void)> testReadWriteDMA =
        boost::bind(&DummyBackendTest::testReadWriteMultiWordRegister,
                    DummyBackendTest, &DummyBackend::readDMA);*/

		add(BOOST_TEST_CASE(DummyBackendTest::testCalculateVirtualAddress));
		add(BOOST_TEST_CASE(DummyBackendTest::testCheckSizeIsMultipleOfWordSize));
		add(BOOST_TEST_CASE(DummyBackendTest::testAddressRange));
		// add( BOOST_CLASS_TEST_CASE( &DummyBackendTest::testOpenClose,
		// dummyBackendTest ) );
		add(BOOST_CLASS_TEST_CASE(&DummyBackendTest::testReadWriteSingleWordRegister,
				dummyBackendTest));
		add(BOOST_TEST_CASE(testReadWriteArea)); // not BOOST_CLASS_TEST_CASE as the
		// functions are already bound to
		// the instance
		//add(BOOST_TEST_CASE(testReadWriteDMA));
		add(BOOST_CLASS_TEST_CASE(&DummyBackendTest::testWriteDMA, dummyBackendTest));
		add(BOOST_CLASS_TEST_CASE(&DummyBackendTest::testReadDeviceInfo,
				dummyBackendTest));
		add(readOnlyTestCase);
		add(writeCallbackFunctionsTestCase);
		add(writeRegisterWithoutCallbackTestCase);
		add(BOOST_CLASS_TEST_CASE(&DummyBackendTest::testIsWriteRangeOverlap,
				dummyBackendTest));
		add(BOOST_CLASS_TEST_CASE(&DummyBackendTest::testFinalClosing,
				dummyBackendTest));
		add(createBackendTestCase);
		add(openTestCase);
		add (closeTestCase);
		add(openCloseTestCase);
	}
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value = "DummyBackend test suite";
	framework::master_test_suite().add(new DummyBackendTestSuite);

	return NULL;
}

void DummyBackendTest::testCalculateVirtualAddress() {
	BOOST_CHECK(DummyBackend::calculateVirtualAddress(0, 0) == 0UL);
	BOOST_CHECK(DummyBackend::calculateVirtualAddress(0x35, 0) == 0x35UL);
	BOOST_CHECK(DummyBackend::calculateVirtualAddress(0x67875, 0x3) ==
			0x3000000000067875UL);
	BOOST_CHECK(DummyBackend::calculateVirtualAddress(0, 0x4) ==
			0x4000000000000000UL);

	// the first bit of the bar has to be cropped
	BOOST_CHECK(DummyBackend::calculateVirtualAddress(0x123, 0xD) ==
			0x5000000000000123UL);
}

void DummyBackendTest::testCheckSizeIsMultipleOfWordSize() {
	// just some arbitrary numbers to test %4 = 0, 1, 2, 3
	BOOST_CHECK_NO_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(24));

	BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(25),
			DummyBackendException);

	BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(26),
			DummyBackendException);

	BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(27),
			DummyBackendException);
}

void DummyBackendTest::testReadWriteSingleWordRegister() {
	// freshlyopenice();
	TestableDummyBackend* dummyBackend = getBackendInstance(true);
	RegisterInfoMap::RegisterInfo mappingElement;
	dummyBackend->_registerMapping->getRegisterInfo(CLOCK_RESET_REGISTER_STRING,
			mappingElement);
	uint32_t offset = mappingElement.reg_address;
	uint8_t bar = mappingElement.reg_bar;
	int32_t dataContent = -1;
	//BOOST_CHECK_NO_THROW(dummyBackend->readReg(bar, offset, &dataContent));
	BOOST_CHECK_NO_THROW(dummyBackend->read(bar, offset, &dataContent,4));
	BOOST_CHECK(dataContent == 0);
	dataContent = 47;
	BOOST_CHECK_NO_THROW(dummyBackend->write(bar, offset, &dataContent,4));
	dataContent = -1; // make sure the value is really being read
	// no need to test NO_THROW on the same register twice
	//dummyBackend->readReg(offset, &dataContent, bar);
	dummyBackend->read(bar, offset, &dataContent, 4);
	BOOST_CHECK(dataContent == 47);

	// the size as index is invalid, allowed range is 0..size-1 included.
	BOOST_CHECK_THROW(dummyBackend->read(bar,
					     dummyBackend->_barContents[bar].size()*sizeof(int32_t),
					     &dataContent, 4),
			  DummyBackendException);
	BOOST_CHECK_THROW(dummyBackend->write(bar,
			dummyBackend->_barContents[bar].size() * sizeof(int32_t),
			&dataContent, 4),
			DummyBackendException);
}
void DummyBackendTest::testReadWriteMultiWordRegister(
		void (DummyBackend::*readFunction)(uint8_t, uint32_t, int32_t*, size_t)) {
	// freshlyopenice();
	TestableDummyBackend* dummyBackend = getBackendInstance(true);
	RegisterInfoMap::RegisterInfo mappingElement;
	dummyBackend->_registerMapping->getRegisterInfo(CLOCK_MUX_REGISTER_STRING,
			mappingElement);

	uint32_t offset = mappingElement.reg_address;
	uint8_t bar = mappingElement.reg_bar;
	size_t sizeInBytes = mappingElement.reg_size;
	size_t sizeInWords = mappingElement.reg_size / sizeof(int32_t);
	std::vector<int32_t> dataContent(sizeInWords, -1);

	BOOST_CHECK_NO_THROW((dummyBackend->*readFunction)(bar, offset, &(dataContent[0]),
			sizeInBytes));
	for (std::vector<int32_t>::iterator dataIter = dataContent.begin();
			dataIter != dataContent.end(); ++dataIter) {
		std::stringstream errorMessage;
		errorMessage << "*dataIter = " << *dataIter;
		BOOST_CHECK_MESSAGE(*dataIter == 0, errorMessage.str());
	}

	for (unsigned int index = 0; index < dataContent.size(); ++index) {
		dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
	}
	BOOST_CHECK_NO_THROW(
			dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes));
	// make sure the value is really being read
	std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);

	// no need to test NO_THROW on the same register twice
	dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);

	// make sure the value is really being read
	for (unsigned int index = 0; index < dataContent.size(); ++index) {
		BOOST_CHECK(dataContent[index] ==
				static_cast<int32_t>((index + 1) * (index + 1)));
	}

	// tests for exceptions
	// 1. base address too large
	BOOST_CHECK_THROW(dummyBackend->read(bar,
			dummyBackend->_barContents[bar].size() * sizeof(int32_t),
			//&(dataContent[0]), sizeInBytes, bar),
			&(dataContent[0]),  sizeInBytes),
			DummyBackendException);
	BOOST_CHECK_THROW(dummyBackend->write(bar,
			dummyBackend->_barContents[bar].size() * sizeof(int32_t),
			&(dataContent[0]), sizeInBytes),
			DummyBackendException);
	// 2. size too large (works because the target register is not at offfset 0)
	// resize the data vector for this test
	dataContent.resize(dummyBackend->_barContents[bar].size());
	BOOST_CHECK_THROW(dummyBackend->read(bar,
			offset, &(dataContent[0]),
			//dummyBackend->_barContents[bar].size() * sizeof(int32_t),bar),
			dummyBackend->_barContents[bar].size() * sizeof(int32_t)),
			DummyBackendException);
	BOOST_CHECK_THROW(dummyBackend->write(bar,
			offset, &(dataContent[0]),
			dummyBackend->_barContents[bar].size() * sizeof(int32_t)
	),
			DummyBackendException);
	// 3. size not multiple of 4
	BOOST_CHECK_THROW(
			//dummyBackend->readArea(offset, &(dataContent[0]), sizeInBytes - 1, bar),
			dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes - 1),
			DummyBackendException);
	BOOST_CHECK_THROW(
			dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes - 1),
			DummyBackendException);
}

void DummyBackendTest::freshlyopenice() {
	try {
		_dummyBackend->open();
	}
	catch (DummyBackendException&) {
		// make sure the backend was freshly opened, so
		// registers are set to 0.
		_dummyBackend->close();
		_dummyBackend->open();
	}
}

void DummyBackendTest::testWriteDMA() {
	// will probably never be implemented
	TestableDummyBackend* dummyBackend = getBackendInstance();
	BOOST_CHECK_THROW(dummyBackend->writeDMA(0, 0, NULL, 0),
			NotImplementedException);
}

TestableDummyBackend* DummyBackendTest::getBackendInstance(bool reOpen) {
	if (_backendInstance == 0)
		_backendInstance = FactoryInstance.createBackend(EXISTING_DEVICE);
	if (reOpen || (!_backendInstance->isOpen())) {
		if (_backendInstance->isOpen())
			_backendInstance->close();
		_backendInstance->open();
	}
	DeviceBackend * rawBasePointer = _backendInstance.get();
	return (static_cast<TestableDummyBackend*>(rawBasePointer));
}

void DummyBackendTest::testReadDeviceInfo() {
	std::string deviceInfo;
	TestableDummyBackend* dummyBackend = getBackendInstance();
	deviceInfo = dummyBackend->readDeviceInfo();
	std::cout << deviceInfo << std::endl;
	BOOST_CHECK(deviceInfo ==
			(std::string("DummyBackend with mapping file ") +
					TEST_MAPPING_FILE));
}

void DummyBackendTest::testReadOnly() {
	// freshlyopenice();
	TestableDummyBackend* dummyBackend = getBackendInstance(true);
	RegisterInfoMap::RegisterInfo mappingElement;
	dummyBackend->_registerMapping->getRegisterInfo(CLOCK_MUX_REGISTER_STRING,
			mappingElement);

	uint32_t offset = mappingElement.reg_address;
	uint8_t bar = mappingElement.reg_bar;
	size_t sizeInBytes = mappingElement.reg_size;
	size_t sizeInWords = mappingElement.reg_size / sizeof(int32_t);
	std::stringstream errorMessage;
	errorMessage << "This register should have 4 words. "
			<< "If you changed your mapping you have to adapt "
			<< "the testReadOnly() test.";
	BOOST_REQUIRE_MESSAGE(sizeInWords == 4, errorMessage.str());

	std::vector<int32_t> dataContent(sizeInWords);
	for (unsigned int index = 0; index < dataContent.size(); ++index) {
		dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
	}
	dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes);
	dummyBackend->setReadOnly(bar, offset, 1);

	// the actual test: write 42 to all registers, register 0 must not change, all
	// others have to
	std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 42);
	dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes);
	std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
	dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);
	BOOST_CHECK(dataContent[0] == 1);
	BOOST_CHECK(dataContent[1] == 42);
	BOOST_CHECK(dataContent[2] == 42);
	BOOST_CHECK(dataContent[3] == 42);
	// also set the last two words to read only. Now only the second word has to
	// change.
	// We use the addressRange interface for it to also cover this.
	TestableDummyBackend::AddressRange lastTwoMuxRegisters(bar,
			offset + 2 * sizeof(int32_t), 2 * sizeof(int32_t));
	dummyBackend->setReadOnly(lastTwoMuxRegisters);
	std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 29);
	// also test with single write operations
	for (size_t index = 0; index < sizeInWords; ++index) {
		dummyBackend->write(bar, offset + index * sizeof(int32_t), &dataContent[index],4);
	}

	std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
	dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);
	BOOST_CHECK(dataContent[0] == 1);
	BOOST_CHECK(dataContent[1] == 29);
	BOOST_CHECK(dataContent[2] == 42);
	BOOST_CHECK(dataContent[3] == 42);

	// check that the next register is still writeable (boundary test)
	int32_t originalNextDataWord;
	dummyBackend->read(bar, offset + sizeInBytes, &originalNextDataWord, 4);
	int32_t writeWord = originalNextDataWord + 1 ;
	dummyBackend->write(bar, offset + sizeInBytes, &writeWord , 4);
	int32_t readbackWord;
	dummyBackend->read(bar, offset + sizeInBytes, &readbackWord, 4);
	BOOST_CHECK(originalNextDataWord + 1 == readbackWord);
}

void DummyBackendTest::testWriteCallbackFunctions() {
	// We just require the first bar to be 12 registers long.
	// Everything else would overcomplicate this test. For a real
	// application one would always use register names from mapping,
	// but this is not the purpose of this test.

	// from the previous test we know that adresses 32, 40 and 44 are write only
	TestableDummyBackend* dummyBackend = getBackendInstance();
	BOOST_REQUIRE(dummyBackend->_barContents[0].size() >= 13);
	a = 0;
	b = 0;
	c = 0;
	dummyBackend->setWriteCallbackFunction(
			TestableDummyBackend::AddressRange(0,36, 4),
			boost::bind(&DummyBackendTest::increaseA, this));
	dummyBackend->setWriteCallbackFunction(
			TestableDummyBackend::AddressRange(0, 28, 24),
			boost::bind(&DummyBackendTest::increaseB, this));
	dummyBackend->setWriteCallbackFunction(
			TestableDummyBackend::AddressRange(0, 20, 12),
			boost::bind(&DummyBackendTest::increaseC, this));

	// test single writes
	int32_t dataWord(42);
	dummyBackend->write(0, 12, &dataWord, 4); // nothing
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 0);
	BOOST_CHECK(c == 0);

	dummyBackend->write(0, 20, &dataWord, 4); // c
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 0);
	BOOST_CHECK(c == 1);
	dummyBackend->write(0, 24, &dataWord, 4); // c
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 0);
	BOOST_CHECK(c == 2);
	dummyBackend->write(0, 28, &dataWord, 4); // bc
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 1);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 32, &dataWord, 4); // read only
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 1);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 36, &dataWord, 4); // ab
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 2);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 40, &dataWord, 4); // read only
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 2);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 44, &dataWord, 4); // read only
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 2);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 48, &dataWord, 4); // b
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 3);
	BOOST_CHECK(c == 3);

	std::vector<int32_t> dataContents(8, 42); // eight words, each with content 42
	a = 0;
	b = 0;
	c = 0;
	dummyBackend->write(0, 20, &(dataContents[0]), 32); // abc
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 1);
	BOOST_CHECK(c == 1);
	dummyBackend->write(0, 20, &(dataContents[0]), 8); // c
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 1);
	BOOST_CHECK(c == 2);
	dummyBackend->write(0, 20, &(dataContents[0]), 12); // bc
	BOOST_CHECK(a == 1);
	BOOST_CHECK(b == 2);
	BOOST_CHECK(c == 3);
	dummyBackend->write(0, 28, &(dataContents[0]), 24); // abc
	BOOST_CHECK(a == 2);
	BOOST_CHECK(b == 3);
	BOOST_CHECK(c == 4);
	dummyBackend->write(0, 32, &(dataContents[0]), 16); // ab
	BOOST_CHECK(a == 3);
	BOOST_CHECK(b == 4);
	BOOST_CHECK(c == 4);
	dummyBackend->write(0, 40, &(dataContents[0]), 8); // readOnly
	BOOST_CHECK(a == 3);
	BOOST_CHECK(b == 4);
	BOOST_CHECK(c == 4);
	dummyBackend->write(0, 4, &(dataContents[0]), 8); // nothing
	BOOST_CHECK(a == 3);
	BOOST_CHECK(b == 4);
	BOOST_CHECK(c == 4);
}

void DummyBackendTest::testWriteRegisterWithoutCallback() {
	a = 0;
	b = 0;
	c = 0;
	int32_t dataWord = 42;
	TestableDummyBackend* dummyBackend = getBackendInstance();
	dummyBackend->writeRegisterWithoutCallback(0,
			20, dataWord); // c has callback installed on this register
	BOOST_CHECK(a == 0);
	BOOST_CHECK(b == 0);
	BOOST_CHECK(c == 0); // c must not change

	// read only is also disabled for this internal function
	dummyBackend->read(0, 40, &dataWord, 4);
	dummyBackend->writeRegisterWithoutCallback(0, 40, dataWord + 1);
	int32_t readbackDataWord;
	dummyBackend->read(0, 40, &readbackDataWord, 4);
	BOOST_CHECK(readbackDataWord == dataWord + 1);
}

void DummyBackendTest::testAddressRange() {
	TestableDummyBackend::AddressRange range24_8_0(0, 24, 8);

	BOOST_CHECK(range24_8_0.offset == 24);
	BOOST_CHECK(range24_8_0.sizeInBytes == 8);
	BOOST_CHECK(range24_8_0.bar == 0);

	TestableDummyBackend::AddressRange range24_8_1(1, 24, 8); // larger bar
	TestableDummyBackend::AddressRange range12_8_1(1, 12, 8); // larger bar, smaller offset
	TestableDummyBackend::AddressRange range28_8_0(0, 28, 8); // larger offset
	TestableDummyBackend::AddressRange range28_8_1(1, 28, 8); // larger bar, larger offset
	TestableDummyBackend::AddressRange range24_12_0(0,24, 12); // different size, compares equal with range1

	// compare 24_8_0 with the other cases as left argument
	BOOST_CHECK((range24_8_0 < range24_8_1));
	BOOST_CHECK((range24_8_0 < range12_8_1));
	BOOST_CHECK((range24_8_0 < range28_8_0));
	BOOST_CHECK((range24_8_0 < range28_8_1));
	BOOST_CHECK(!(range24_8_0 < range24_12_0));

	// compare 24_8_0 with the other cases as right argument
	BOOST_CHECK(!(range24_8_1 < range24_8_0));
	BOOST_CHECK(!(range12_8_1 < range24_8_0));
	BOOST_CHECK(!(range28_8_0 < range24_8_0));
	BOOST_CHECK(!(range28_8_1 < range24_8_0));
	BOOST_CHECK(!(range24_12_0 < range24_8_0));
}

void DummyBackendTest::testIsWriteRangeOverlap() {
	// the only test not covered by the writeCallbackFunction test:
	// An overlapping range in different bars
	TestableDummyBackend* dummyBackend = getBackendInstance();
	bool overlap = dummyBackend->isWriteRangeOverlap(
			TestableDummyBackend::AddressRange(0, 0, 12),
			TestableDummyBackend::AddressRange(1, 0, 12));
	BOOST_CHECK(overlap == false);
}

void DummyBackendTest::testFinalClosing() {
	// all features have to be enabled before closing
	TestableDummyBackend* dummyBackend = getBackendInstance();
	BOOST_CHECK(dummyBackend->_barContents.size() != 0);
	BOOST_CHECK(dummyBackend->_readOnlyAddresses.size() != 0);
	BOOST_CHECK(dummyBackend->_writeCallbackFunctions.size() != 0);

	dummyBackend->close();

	// all features lists have to be empty now
	BOOST_CHECK(dummyBackend->_readOnlyAddresses.size() == 0);
	BOOST_CHECK(dummyBackend->_writeCallbackFunctions.size() == 0);
}

void DummyBackendTest::testOpenClose() {

	TestableDummyBackend* dummyBackend = getBackendInstance(true);
	// there have to be bars 0 and 2  with sizes 0x14C and 0x1000 bytes,
	// plus the dma bar 0xD
	// BOOST_CHECK((*dummyBackend)._barContents.size() == 3 );
	BOOST_CHECK(dummyBackend->_barContents.size() == 3);
	std::map<uint8_t, std::vector<int32_t> >::const_iterator bar0Iter =
			dummyBackend->_barContents.find(0);
	BOOST_REQUIRE(bar0Iter != dummyBackend->_barContents.end());
	BOOST_CHECK(bar0Iter->second.size() == 0x53); // 0x14C bytes in 32 bit words
	std::map<uint8_t, std::vector<int32_t> >::const_iterator bar2Iter =
			dummyBackend->_barContents.find(2);
	BOOST_REQUIRE(bar2Iter != dummyBackend->_barContents.end());
	BOOST_CHECK(bar2Iter->second.size() == 0x400); // 0x1000 bytes in 32 bit words

	// the "prtmapFile" has an implicit converion to bool to check
	// if it points to NULL
	BOOST_CHECK(dummyBackend->_registerMapping);
	BOOST_CHECK(dummyBackend->isOpen());
	BOOST_CHECK_THROW(dummyBackend->open(),
			DummyBackendException);

	dummyBackend->close();
	BOOST_CHECK(dummyBackend->isOpen() == false);
	BOOST_CHECK_THROW(dummyBackend->close(), DummyBackendException);
}

void DummyBackendTest::testClose() {
	/** Try closing the backend */
	_backendInstance->close();
	/** backend should not be open now */
	BOOST_CHECK(_backendInstance->isOpen() == false);
	/** backend should remain in connected state */
	BOOST_CHECK(_backendInstance->isConnected() == true);
}

void DummyBackendTest::testOpen() {
	_backendInstance->open();
	BOOST_CHECK(_backendInstance->isOpen() == true);
	BOOST_CHECK(_backendInstance->isConnected() == true);
}

void DummyBackendTest::testCreateBackend() {
  /** Only for testing purpose */
  std::list <std::string> pararmeters;
  BOOST_CHECK_THROW(DummyBackend::createInstance("","",pararmeters),DummyBackendException);
	/** Try creating a non existing backend */
	BOOST_CHECK_THROW(FactoryInstance.createBackend(NON_EXISTING_DEVICE),BackendFactoryException);
	/** Try creating an existing backend */
	_backendInstance = FactoryInstance.createBackend(EXISTING_DEVICE);
	BOOST_CHECK(_backendInstance);
	/** backend should be in connect state now */
	BOOST_CHECK(_backendInstance->isConnected() == true);
	/** backend should not be in open state */
	BOOST_CHECK(_backendInstance->isOpen() == false);
}
