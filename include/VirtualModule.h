/*
 * VirtualModule.h
 *
 *  Created on: Apr 4, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VIRTUAL_MODULE_H
#define CHIMERATK_VIRTUAL_MODULE_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"
#include <ChimeraTK/RegisterPath.h>

namespace ChimeraTK {

/** A virtual module generated by EntityOwner::findTag(). */
class VirtualModule : public Module {

public:
  /** Constructor */
  VirtualModule(const std::string &name, const std::string &description,
                ModuleType moduleType)
      : Module(nullptr, name, description), _moduleType(moduleType) {
    if (name.find_first_of("/") != std::string::npos) {
      throw ChimeraTK::logic_error("Module names must not contain slashes: '" +
                                   name + "'.");
    }
  }

  /** Copy constructor */
  VirtualModule(const VirtualModule &other);

  /** Assignment operator */
  VirtualModule &operator=(const VirtualModule &other);

  /** Destructor */
  virtual ~VirtualModule();

  VariableNetworkNode
  operator()(const std::string &variableName) const override;

  Module &operator[](const std::string &moduleName) const override;

  void connectTo(const Module &target,
                 VariableNetworkNode trigger = {}) const override;

  /** Add an accessor. The accessor instance will be added to an internal list.
   */
  void addAccessor(VariableNetworkNode accessor);

  /** Add a virtual sub-module. The module instance will be added to an internal
   * list. */
  void addSubModule(VirtualModule module);

  /** Return the submodule with the given name. If it doesn't exist, create it
   * first. */
  VirtualModule &createAndGetSubmodule(const RegisterPath &moduleName);

  /** Like createAndGetSubmodule(), but recursively create a hierarchy of
   * submodules separated by "/" in the moduleName. */
  VirtualModule &createAndGetSubmoduleRecursive(const RegisterPath &moduleName);

  ModuleType getModuleType() const override { return _moduleType; }

  const Module &virtualise() const override;

protected:
  std::list<VirtualModule> submodules;
  ModuleType _moduleType;
};

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VIRTUAL_MODULE_H */
