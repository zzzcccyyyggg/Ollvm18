## 前言

在LLVM项目外以插件的形式编译LLVM，并移植到最新的LLVM18上

## 更改

更改  `include/llvm/IR/Function.h` 目录下找到 `getBasicBlockList()` 函数的声明，并将其从私有部分移动到公共部分。

类似下方示例

```cpp
class Function {
  ...
private:
  BasicBlockListType &getBasicBlockList() { return BasicBlocks; }
  const BasicBlockListType &getBasicBlockList() const { return BasicBlocks; }
  ...
};
```

要将其更改为公共方法，您应该这样修改：

```cpp
class Function {
  ...
public:
  BasicBlockListType &getBasicBlockList() { return BasicBlocks; }
  const BasicBlockListType &getBasicBlockList() const { return BasicBlocks; }
  ...
};
```

对于

IndirectBranch.cpp

IndirectCall.cpp

IndirectGlobalVariable.cpp

将其中如`Type::getInt8PtrTy()`等相应API更改如下

```
llvm::PointerType::get(Type::getInt8Ty(F.getContext()),0))
```

增添plugin.cpp

```cpp
// 引用 Obfuscation 相关文件
#include "BogusControlFlow.h" // 虚假控制流
#include "Flattening.h"  // 控制流平坦化
#include "SplitBasicBlock.h" // 基本块分割
#include "Substitution.h" // 指令替换
#include "StringEncryption.h" // 字符串加密
#include "IndirectGlobalVariable.h" // 间接全局变量
#include "IndirectBranch.h" // 间接跳转
#include "IndirectCall.h" // 间接调用
#include "Utils.h" // 为了控制函数名混淆开关 (bool obf_function_name_cmd;)
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
using namespace llvm;
bool s_obf_split = false;
bool s_obf_sobf = false;
bool s_obf_fla = false;
bool s_obf_sub = false;
bool s_obf_bcf = false;
bool s_obf_ibr = false;
bool s_obf_igv = false;
bool s_obf_icall = false;
bool s_obf_fn_name_cmd = false;

void parseEnvironmentOptions() {
    const char* env;

    env = getenv("OBF_SPLIT");
    if (env ) {
        s_obf_split = true;
    }

    env = getenv("OBF_SOBF");
    if (env ) {
        s_obf_sobf = true;
    }

    env = getenv("OBF_FLA");
    if (env ) {
        s_obf_fla = true;
    }

    env = getenv("OBF_SUB");
    if (env ) {
        s_obf_sub = true;
    }

    env = getenv("OBF_BCF");
    if (env) {
      s_obf_bcf = true;
    }

    env = getenv("OBF_IBR");
    if (env) {
        s_obf_ibr = true;
    }

    env = getenv("OBF_IGV");
    if (env ) {
        s_obf_igv = true;
    }

    env = getenv("OBF_ICALL");
    if (env) {
        s_obf_icall = true;
    }

    env = getenv("OBF_FNCMD");
    if (env) {
        s_obf_fn_name_cmd = true;
    }
    else{
      outs() << "[Soule] OBF_FNCMD is not set\n";
    }
}
llvm::PassPluginLibraryInfo getObfuscationPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "Obfuscation", LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        // Soule
        outs() << "[Soule] registerPipelineStartEPCallback\n";
        parseEnvironmentOptions();
        PB.registerPipelineStartEPCallback(
            [](llvm::ModulePassManager &MPM,
              llvm::OptimizationLevel Level) {
              outs() << "[Soule] run.PipelineStartEPCallback\n";
              obf_function_name_cmd = s_obf_fn_name_cmd;
              if (obf_function_name_cmd) {
                outs() << "[Soule] enable function name control obfuscation(_ + command + _ | example: function_fla_)\n";
              }
              MPM.addPass(StringEncryptionPass(s_obf_sobf)); // 先进行字符串加密 出现字符串加密基本块以后再进行基本块分割和其他混淆 加大解密难度
              llvm::FunctionPassManager FPM;
              FPM.addPass(IndirectCallPass(s_obf_icall)); // 间接调用
              FPM.addPass(SplitBasicBlockPass(s_obf_split)); // 优先进行基本块分割
              FPM.addPass(FlatteningPass(s_obf_fla)); // 对于控制流平坦化
              FPM.addPass(SubstitutionPass(s_obf_sub)); // 指令替换
              FPM.addPass(BogusControlFlowPass(s_obf_bcf)); // 虚假控制流
              MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
              MPM.addPass(IndirectBranchPass(s_obf_ibr)); // 间接指令 理论上间接指令应该放在最后
              MPM.addPass(IndirectGlobalVariablePass(s_obf_igv)); // 间接全局变量
              // MPM.addPass(RewriteSymbolPass()); // 根据yaml信息 重命名特定symbols
            }
        );
      }};
}

#ifndef LLVM_OBFUSCATION_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getObfuscationPluginInfo();
}
#endif
```

更改CMakeLists如下

```cmake
cmake_minimum_required(VERSION 3.6)
project(Obfuscation)

#CflLLVMPass.so 
# LLVM uses C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/home/zzzccc/llvm-project/")
find_package(LLVM REQUIRED CONFIG)
# Include the part of LLVM's CMake libraries that defines
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS} "./include")
link_directories(${LLVM_LIBRARY_DIRS})
# Our pass lives in this subdirectory.
add_subdirectory(Obfuscation)
```



```cmake
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

```

## 使用命令参考

```sh
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_SPLIT=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_SOBF=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_FLA=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_SUB=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_BCF=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_IBR=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_IGV=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_ICALL=1
zzzccc@LAPTOP-D9NHVOAF:~/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation$ export OBF_FNCMD=1

/home/zzzccc/llvm-project/bin/clang  -Wl,--unresolved-symbols=ignore-all -fpass-plugin=`echo /home/zzzccc/ollvm17/llvm-project/llvm/lib/Passes/build/Obfuscation/Shield.*` -O0 /home/zzzccc/Shield/tmp/test.c -o ./a
```

## 源项目

https://github.com/DreamSoule/ollvm17

## todo

还需要将部分命令行选项改为环境变量的输入方式