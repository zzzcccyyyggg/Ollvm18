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