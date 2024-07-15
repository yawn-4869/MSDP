cmake_minimum_required(VERSION 3.0)
project(MSDP)

# 设置 C++11 + pthread 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

# 如果想要生成的可执行文件拥有符号表，可以gdb调试，就直接加上这句
add_compile_options(-Wall -g)

# 设置编译器为 g++
set(CMAKE_CXX_COMPILER g++)

# 查找 Protobuf 库
find_package(Protobuf REQUIRED)
find_package(cppzmq)
find_library(ROCKET_LIB rocket PATHS /usr/local/lib)
find_library(TINYXML_LIB tinyxml PATHS /usr/lib)

if(ROCKET_LIB)
    message(STATUS "Found librocket: ${ROCKET_LIB}")
else()
    message(FATAL_ERROR "librocket not found")
endif()

if(TINYXML_LIB)
    message(STATUS "Found libtinyxml: ${TINYXML_LIB}")
else()
    message(FATAL_ERROR "libtinyxml not found")
endif()


# 设置变量，下面的代码都可以用到
set(MSDP_ROOT_DIR ${PROJECT_SOURCE_DIR}/msdp)
set(MSDP_COMMON_DIR ${PROJECT_SOURCE_DIR}/msdp/common)
set(MSDP_FUSION_DIR ${PROJECT_SOURCE_DIR}/msdp/fusion)
set(MSDP_NET_DIR ${PROJECT_SOURCE_DIR}/msdp/net)
set(MSDP_TOOLS_DIR ${PROJECT_SOURCE_DIR}/msdp/tools)
set(MSDP_PROTO_DIR ${PROJECT_SOURCE_DIR}/msdp/proto)
# 添加生成的 .pb.h 和 .pb.cc 文件的输出目录
set(MSDP_PROTO_OUT ${PROJECT_SOURCE_DIR}/msdp/proto_out)

# 需要调用的rpc所在文件夹
set(LIB_RPC_DIR ${PROJECT_SOURCE_DIR}/raft_server)
set(LIB_RPC_COMM ${LIB_RPC_DIR}/comm)
set(LIB_RPC_INTERFACE ${LIB_RPC_DIR}/interface)
set(LIB_RPC_PB ${LIB_RPC_DIR}/pb)
set(LIB_RPC_SERVICE ${LIB_RPC_DIR}/service)
set(LIB_RPC_STUBS ${LIB_RPC_DIR}/stubs)

set(BUTTON_RPC_DIR ${PROJECT_SOURCE_DIR}/buttonrpc)
set(BUTTON_RPC_DEP ${BUTTON_RPC_DIR}/depends)

# 添加 .proto 文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${MSDP_PROTO_DIR}/fusion.proto ${MSDP_PROTO_DIR}/raft.proto)

# 收集所有子目录的源文件
file(GLOB_RECURSE MSDP_SOURCES
    ${MSDP_ROOT_DIR}/*.cpp
    ${MSDP_COMMON_DIR}/*.cpp
    ${MSDP_FUSION_DIR}/*.cpp
    ${MSDP_NET_DIR}/*.cpp
    ${MSDP_TOOLS_DIR}/*.cpp
)

file (GLOB_RECURSE LIB_RPC_SOURCES
    ${LIB_RPC_COMM}/*.cc
    ${LIB_RPC_INTERFACE}/*.cc
    ${LIB_RPC_PB}/*.cc
    ${LIB_RPC_SERVICE}/*.cc
    ${LIB_RPC_STUBS}/*.cc
)

# file (GLOB_RECURSE BUTTON_RPC_SOURCES
#     ${BUTTON_RPC_DEP}/*.hpp
#     ${BUTTON_RPC_DIR}/*.hpp
# )

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 生成rpc静态库
add_library(librpc STATIC ${LIB_RPC_SOURCES})
# 包含静态库路径
link_directories(${PROJECT_SOURCE_DIR}/lib)
# 链接静态库
link_libraries(librpc)
# 生成可执行文件
add_executable(msdp_app ${MSDP_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

# 规定.h头文件路径
target_include_directories(msdp_app PRIVATE ${MSDP_ROOT_DIR} ${MSDP_COMMON_DIR} 
    ${MSDP_FUSION_DIR} ${MSDP_NET_DIR} ${MSDP_TOOLS_DIR} ${MSDP_PROTO_OUT}
    ${LIB_RPC_COMM} ${LIB_RPC_INTERFACE} ${LIB_RPC_PB} ${LIB_RPC_SERVICE} ${LIB_RPC_STUBS}
    ${BUTTON_RPC_DIR}
)

# 链接 Protobuf 库
target_link_libraries(msdp_app cppzmq ${PROTOBUF_LIBRARIES} ${ROCKET_LIB} ${TINYXML_LIB})
