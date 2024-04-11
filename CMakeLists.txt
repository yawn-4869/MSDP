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

# 设置变量，下面的代码都可以用到
set(MSDP_ROOT_DIR ${PROJECT_SOURCE_DIR}/msdp)
set(MSDP_COMMON_DIR ${PROJECT_SOURCE_DIR}/msdp/common)
set(MSDP_FUSION_DIR ${PROJECT_SOURCE_DIR}/msdp/fusion)
set(MSDP_NET_DIR ${PROJECT_SOURCE_DIR}/msdp/net)
set(MSDP_TOOLS_DIR ${PROJECT_SOURCE_DIR}/msdp/tools)
set(MSDP_PROTO_DIR ${PROJECT_SOURCE_DIR}/msdp/proto)
# 添加生成的 .pb.h 和 .pb.cc 文件的输出目录
set(MSDP_PROTO_OUT ${PROJECT_SOURCE_DIR}/msdp/proto_out)

# 添加 .proto 文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${MSDP_PROTO_DIR}/fusion.proto)

# 编译子文件夹的CMakeLists.txt
# add_subdirectory(proto_pb2)

# 收集所有子目录的源文件
file(GLOB_RECURSE MSDP_SOURCES
    ${MSDP_ROOT_DIR}/*.cpp
    ${MSDP_COMMON_DIR}/*.cpp
    ${MSDP_FUSION_DIR}/*.cpp
    ${MSDP_NET_DIR}/*.cpp
    ${MSDP_TOOLS_DIR}/*.cpp
)

# 生成可执行文件
add_executable(msdp_app ${MSDP_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

# 规定.h头文件路径
target_include_directories(msdp_app PRIVATE ${MSDP_ROOT_DIR} ${MSDP_COMMON_DIR} 
    ${MSDP_FUSION_DIR} ${MSDP_NET_DIR} ${MSDP_TOOLS_DIR} ${MSDP_PROTO_OUT}
)

# 链接 Protobuf 库
target_link_libraries(msdp_app ${PROTOBUF_LIBRARIES})
