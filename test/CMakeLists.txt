cmake_minimum_required(VERSION 3.8)  # 不要修改
project(test)                   # 不要修改

set(CMAKE_CXX_STANDARD      17)      # 不要修改
set(CMAKE_C_STANDARD        11)      # 不要修改

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/) # 不要修改
option(BUILD_SHARED_LIBS    ""  OFF) # 不要修改

aux_source_directory(./                     cur_src) # 不要修改

# 如果需要，可以使用aux_source_directory增加目录

add_executable(test                   ${cur_src})

# 指定静态库的路径
set(MYLIB_PATH "${CMAKE_SOURCE_DIR}/../tools/libLogTool.a")
target_link_libraries(test PRIVATE ${MYLIB_PATH})

