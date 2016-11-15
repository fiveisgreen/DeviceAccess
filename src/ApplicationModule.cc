/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationModule.h"
#include "Accessor.h"

namespace ChimeraTK {

  void ApplicationModule::run() {

    // start the module thread
    assert(!moduleThread.joinable());
    moduleThread = boost::thread(&ApplicationModule::mainLoopWrapper, this);
  }

/*********************************************************************************************************************/

  void ApplicationModule::terminate() {
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      moduleThread.join();
    }
    assert(!moduleThread.joinable());
  }

/*********************************************************************************************************************/

  ApplicationModule::~ApplicationModule() {
    assert(!moduleThread.joinable());
  }

/*********************************************************************************************************************/

  void ApplicationModule::mainLoopWrapper() {
    // enter the main loop
    mainLoop();
  }

/*********************************************************************************************************************/

  VariableNetworkNode ApplicationModule::operator()(const std::string& variableName) {
    for(auto variable : getAccessorList()) {
      if(variable->getName() == variableName) return VariableNetworkNode(*variable);
    }
    throw std::logic_error("Variable '"+variableName+"' is not part of the module '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& ApplicationModule::operator[](const std::string& moduleName) {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the module '"+_name+"'.");
  }

} /* namespace ChimeraTK */
