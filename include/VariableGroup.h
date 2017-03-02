/*
 * VariableGroup.h
 *
 *  Created on: Nov 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_GROUP_H
#define CHIMERATK_VARIABLE_GROUP_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {

  class VariableGroup : public Module {

    public:
      
      using Module::Module;

      /** Destructor */
      virtual ~VariableGroup();
      
      /** Wait for receiving an update for any of the push-type variables in the group. Any poll-type variables are
       *  read after receiving the update. If no push-type variables are in the group, this function will just read
       *  all variables. The returned TransferElement will be the push-type variable which has been updated. */
      boost::shared_ptr<mtca4u::TransferElement> readAny();
      
      /** Just call read() on all variables in the group. If there are push-type variables in the group, this call
       *  will block until all of the variables have received an update. */
      void readAll();

      VariableNetworkNode operator()(const std::string& variableName);

      Module& operator[](const std::string& moduleName);

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_GROUP_H */
