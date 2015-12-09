#include <mtca4u/Device.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// We do not need to deal with addresses any more but use the 
// register and module name
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main(){
  // Before you use a device you have to tell the factory 
  // which dmap file to use.
  // \todo Fixme: we use one from the unit tests. examples should have its own
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);

  // Entry in dmap file is
  // PCIE2  sdm://./pci:mtcadummys0; mtcadummy.map
  boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
  myDevice->open("PCIE2");

  // read and print a data word from a register
  int32_t dataWord;
  myDevice->readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  // A bit clumsy: As write can take multiple words we have to pass a 
  // pointer.
  // Read the documentation  mtca4u::Device::readReg  if you
  // want to use arrays!
  int32_t writeWord = dataWord + 42;
  myDevice->writeReg(REGISTER_NAME, MODULE_NAME, &writeWord);
  myDevice->readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->close();

  return 0;
}
