// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_IPC_FUZZER_MESSAGE_LIB_MESSAGE_NAMES_H_
#define TOOLS_IPC_FUZZER_MESSAGE_LIB_MESSAGE_NAMES_H_

#include <map>
#include <string>
#include "base/basictypes.h"
#include "base/logging.h"

namespace ipc_fuzzer {

class MessageNames {
 public:
  MessageNames();
  ~MessageNames();
  static MessageNames* GetInstance();

  void Add(uint32 type, const std::string& name) {
    name_map_[type] = name;
    type_map_[name] = type;
  }

  bool TypeExists(uint32 type) {
    return name_map_.find(type) != name_map_.end();
  }

  bool NameExists(const std::string& name) {
    return type_map_.find(name) != type_map_.end();
  }

  const std::string& TypeToName(uint32 type) {
    TypeToNameMap::iterator it = name_map_.find(type);
    CHECK(it != name_map_.end());
    return it->second;
  }

  uint32 NameToType(const std::string& name) {
    NameToTypeMap::iterator it = type_map_.find(name);
    CHECK(it != type_map_.end());
    return it->second;
  }

 private:
  typedef std::map<uint32, std::string> TypeToNameMap;
  typedef std::map<std::string, uint32> NameToTypeMap;
  TypeToNameMap name_map_;
  NameToTypeMap type_map_;

  static MessageNames* all_names_;

  DISALLOW_COPY_AND_ASSIGN(MessageNames);
};

}  // namespace ipc_fuzzer

#endif  // TOOLS_IPC_FUZZER_MESSAGE_LIB_MESSAGE_NAMES_H_
