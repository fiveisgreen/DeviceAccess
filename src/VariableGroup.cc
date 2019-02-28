/*
 * VariableGroup.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationCore.h"

namespace ChimeraTK {

  VariableGroup::VariableGroup(EntityOwner* owner, const std::string& name, const std::string& description,
      HierarchyModifier hierarchyModifier, const std::unordered_set<std::string>& tags)
  : ModuleImpl(owner, name, description, hierarchyModifier, tags) {
    if(!dynamic_cast<ApplicationModule*>(owner) && !dynamic_cast<DeviceModule*>(owner) &&
        !dynamic_cast<VariableGroup*>(owner)) {
      throw ChimeraTK::logic_error("VariableGroups must be owned by ApplicationModule, DeviceModule or "
                                   "other VariableGroups!");
    }
  }

  VariableGroup::VariableGroup(EntityOwner* owner, const std::string& name, const std::string& description,
      bool eliminateHierarchy, const std::unordered_set<std::string>& tags)
  : ModuleImpl(owner, name, description, eliminateHierarchy, tags) {
    if(!dynamic_cast<ApplicationModule*>(owner) && !dynamic_cast<DeviceModule*>(owner) &&
        !dynamic_cast<VariableGroup*>(owner)) {
      throw ChimeraTK::logic_error("VariableGroups must be owned by ApplicationModule, DeviceModule or "
                                   "other VariableGroups!");
    }
  }

  ConfigReader& VariableGroup::appConfig() const {
    if(dynamic_cast<ApplicationModule*>(getOwner())) {
      return dynamic_cast<ApplicationModule*>(getOwner())->appConfig();
    }
    else if(dynamic_cast<VariableGroup*>(getOwner())) {
      return dynamic_cast<ApplicationModule*>(getOwner())->appConfig();
    }
    else {
      throw ChimeraTK::logic_error(
          "VariableGroup::appConfig() can not be called when the VariableGroup is owned by a DeviceModule!");
    }
  }

} /* namespace ChimeraTK */
