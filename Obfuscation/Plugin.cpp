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