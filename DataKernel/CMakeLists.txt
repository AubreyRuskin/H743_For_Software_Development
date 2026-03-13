# --- 项目设置 ---
cmake_minimum_required(VERSION 3.10)


# --- 收集源文件 ---
# 注意：file(GLOB_RECURSE) 虽然方便，但在添加或删除文件时，CMake 可能不会自动检测到
# 变化，导致需要手动清理构建目录。在大型项目中，更推荐手动列出所有源文件。
file(GLOB_RECURSE SRC_FILES
    "src/*.c"
    "edp01/src/*.c"
    "edp01/src/ext_box/*.c"
    "edp01/src/sample/*.c"
    "edp03/src/sample/*.c"
    "smvcfg/UnifiedCfg/*.c"
    "smvcfg/cfg/*.c"
    "smvcfg/mxml/*.c"
    "vxworks_compat/src/*.c"
    "suanfa/*.c"
)

# --- 1. 创建静态库目标 ---
# DataKernel 作为主工程子项目编译，不在此处提供独立入口 main.c
add_library(datakernel STATIC ${SRC_FILES})


# --- 2. 为已创建的目标配置属性 ---

# 为 datakernel 添加需要包含的头文件目录
# target_include_directories 是比 include_directories 更现代、更推荐的做法
target_include_directories(datakernel PUBLIC
    include
    include/common
    edp01/include
    edp02/include
    edp03/include
    smvcfg/UnifiedCfg
    smvcfg/cfg
    smvcfg/mxml
    src/61850/inc
    vxworks_compat/include
    src/protect/include
    suanfa
)

# 为 datakernel 添加编译选项
target_compile_options(datakernel PRIVATE
    -Werror=deprecated-declarations
)

message(STATUS "DataKernel will be built as static library target: datakernel")