cmake_minimum_required(VERSION 3.6)
include_directories(../include)
list(APPEND CMAKE_MODULE_PATH "/home/zzzccc/llvm-project/lib/cmake/llvm")
include(AddLLVM)
add_llvm_library(Shield
  MODULE
  Utils.cpp
  CryptoUtils.cpp
  ObfuscationOptions.cpp
  BogusControlFlow.cpp
  IPObfuscationContext.cpp
  Flattening.cpp
  StringEncryption.cpp
  SplitBasicBlock.cpp
  Substitution.cpp
  IndirectBranch.cpp
  IndirectCall.cpp
  IndirectGlobalVariable.cpp
  Plugin.cpp
  ${PROJECT_SOURCE_DIR}

  DEPENDS
  intrinsics_gen
  )
