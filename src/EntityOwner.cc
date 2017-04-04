/*
 * EntityOwner.cc
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#include <cassert>

#include <iostream>

#include "EntityOwner.h"
#include "Module.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  EntityOwner::EntityOwner(EntityOwner *owner, const std::string &name)
  : _name(name), _owner(owner)
  {
    if(owner != nullptr) {
      auto thisMustBeAModule = static_cast<Module*>(this);  /// @todo TODO FIXME this is a bit dangerous...
      owner->registerModule(thisMustBeAModule);
    }
  }

/*********************************************************************************************************************/

  EntityOwner::~EntityOwner() {
    if(_owner != nullptr) {
      auto thisMustBeAModule = static_cast<Module*>(this);  /// @todo TODO FIXME this is a bit dangerous...
      _owner->unregisterModule(thisMustBeAModule);
    }
  }

/*********************************************************************************************************************/

  void EntityOwner::registerModule(Module *module) {
    moduleList.push_back(module);
  }

/*********************************************************************************************************************/

  void EntityOwner::unregisterModule(Module *module) {
    moduleList.remove(module);
  }

/*********************************************************************************************************************/

  std::list<VariableNetworkNode> EntityOwner::getAccessorListRecursive() {
    // add accessors of this instance itself
    std::list<VariableNetworkNode> list = getAccessorList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      auto sublist = submodule->getAccessorListRecursive();
      list.insert(list.end(), sublist.begin(), sublist.end());
    }
    return list;
  }

/*********************************************************************************************************************/

  std::list<Module*> EntityOwner::getSubmoduleListRecursive() {
    // add modules of this instance itself
    std::list<Module*> list = getSubmoduleList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      auto sublist = submodule->getSubmoduleListRecursive();
      list.insert(list.end(), sublist.begin(), sublist.end());
    }
    return list;
  }

/*********************************************************************************************************************/

  VirtualModule EntityOwner::findTag(const std::string &tag, bool eliminateAllHierarchies) const {

    // create new module to return
    VirtualModule module{_name+"{"+tag+"}"};
    
    // add everything matching the tag to the virtual module and return it
    findTagAndAppendToModule(module, tag, eliminateAllHierarchies, true);
    return module;
  }

  /*********************************************************************************************************************/

  void EntityOwner::findTagAndAppendToModule(VirtualModule &module, const std::string &tag, bool eliminateAllHierarchies,
                                             bool eliminateFirstHierarchy) const {
    
    VirtualModule nextmodule{_name+"{"+tag+"}"};
    VirtualModule *moduleToAddTo;
    
    if(!getEliminateHierarchy() && !eliminateAllHierarchies && !eliminateFirstHierarchy) {
      moduleToAddTo = &nextmodule;
      module.addSubModule(nextmodule);
    }
    else {
      moduleToAddTo = &module;
    }
    
    // add nodes to the module if matching the tag
    for(auto node : getAccessorList()) {
      if(node.getTags().count(tag) > 0) {
        moduleToAddTo->registerAccessor(node);
      }
    }

    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      submodule->findTagAndAppendToModule(*moduleToAddTo, tag, eliminateAllHierarchies);
    }
    
  }

  /*********************************************************************************************************************/

  void EntityOwner::dump(const std::string &prefix) const {
    
    if(prefix == "") {
      std::cout << "==== Hierarchy dump of module '" << _name << "':" << std::endl;
    }
    
    for(auto node : getAccessorList()) {
      std::cout << prefix << "+ ";
      node.dump();
    }

    for(auto submodule : getSubmoduleList()) {
      std::cout << prefix << "| " << submodule->getName() << std::endl;
      submodule->dump(prefix+"| ");
    }
    
  }
  
} /* namespace ChimeraTK */
