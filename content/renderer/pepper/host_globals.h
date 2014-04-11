// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_HOST_GLOBALS_H_
#define CONTENT_RENDERER_PEPPER_HOST_GLOBALS_H_

#include "base/compiler_specific.h"
#include "content/renderer/pepper/host_var_tracker.h"
#include "ppapi/shared_impl/callback_tracker.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var_tracker.h"

namespace content {

class PepperPluginInstanceImpl;
class PluginModule;

class HostGlobals : public ppapi::PpapiGlobals {
 public:
  HostGlobals();
  virtual ~HostGlobals();

  // Getter for the global singleton. Generally, you should use
  // PpapiGlobals::Get() when possible. Use this only when you need some
  // host-specific functionality.
  inline static HostGlobals* Get() {
    DCHECK(PpapiGlobals::Get()->IsHostGlobals());
    return static_cast<HostGlobals*>(PpapiGlobals::Get());
  }

  // PpapiGlobals implementation.
  virtual ppapi::ResourceTracker* GetResourceTracker() OVERRIDE;
  virtual ppapi::VarTracker* GetVarTracker() OVERRIDE;
  virtual ppapi::CallbackTracker* GetCallbackTrackerForInstance(
      PP_Instance instance) OVERRIDE;
  virtual ppapi::thunk::PPB_Instance_API* GetInstanceAPI(PP_Instance instance)
      OVERRIDE;
  virtual ppapi::thunk::ResourceCreationAPI* GetResourceCreationAPI(
      PP_Instance instance) OVERRIDE;
  virtual PP_Module GetModuleForInstance(PP_Instance instance) OVERRIDE;
  virtual std::string GetCmdLine() OVERRIDE;
  virtual void PreCacheFontForFlash(const void* logfontw) OVERRIDE;
  virtual void LogWithSource(PP_Instance instance,
                             PP_LogLevel level,
                             const std::string& source,
                             const std::string& value) OVERRIDE;
  virtual void BroadcastLogWithSource(PP_Module module,
                                      PP_LogLevel level,
                                      const std::string& source,
                                      const std::string& value) OVERRIDE;
  virtual ppapi::MessageLoopShared* GetCurrentMessageLoop() OVERRIDE;
  virtual base::TaskRunner* GetFileTaskRunner() OVERRIDE;

  HostVarTracker* host_var_tracker() { return &host_var_tracker_; }

  // PP_Modules ----------------------------------------------------------------

  // Adds a new plugin module to the list of tracked module, and returns a new
  // module handle to identify it.
  PP_Module AddModule(PluginModule* module);

  // Called when a plugin modulde was deleted and should no longer be tracked.
  // The given handle should be one generated by AddModule.
  void ModuleDeleted(PP_Module module);

  // Returns a pointer to the plugin modulde object associated with the given
  // modulde handle. The return value will be NULL if the handle is invalid.
  PluginModule* GetModule(PP_Module module);

  // PP_Instances --------------------------------------------------------------

  // Adds a new plugin instance to the list of tracked instances, and returns a
  // new instance handle to identify it.
  PP_Instance AddInstance(PepperPluginInstanceImpl* instance);

  // Called when a plugin instance was deleted and should no longer be tracked.
  // The given handle should be one generated by AddInstance.
  void InstanceDeleted(PP_Instance instance);

  void InstanceCrashed(PP_Instance instance);

  // Returns a pointer to the plugin instance object associated with the given
  // instance handle. The return value will be NULL if the handle is invalid or
  // if the instance has crashed.
  PepperPluginInstanceImpl* GetInstance(PP_Instance instance);

 private:
  // PpapiGlobals overrides.
  virtual bool IsHostGlobals() const OVERRIDE;

  static HostGlobals* host_globals_;

  ppapi::ResourceTracker resource_tracker_;
  HostVarTracker host_var_tracker_;

  // Tracks all live instances and their associated object.
  typedef std::map<PP_Instance, PepperPluginInstanceImpl*> InstanceMap;
  InstanceMap instance_map_;

  // Tracks all live modules. The pointers are non-owning, the PluginModule
  // destructor will notify us when the module is deleted.
  typedef std::map<PP_Module, PluginModule*> ModuleMap;
  ModuleMap module_map_;

  DISALLOW_COPY_AND_ASSIGN(HostGlobals);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_HOST_GLOBALS_H_
