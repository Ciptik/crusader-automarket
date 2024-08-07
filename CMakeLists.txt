cmake_minimum_required(VERSION 3.30)

project(automarket)

add_library(automarket SHARED src/dllmain.cpp
                              src/log.cpp
                              src/menu.cpp
                              src/psutils.cpp
                              src/signature.cpp
                              src/internal.cpp
)

set_target_properties(automarket PROPERTIES OUTPUT_NAME "dplayx" PREFIX "")

add_subdirectory(src/include/minhook)

target_link_libraries(automarket PRIVATE minhook
                                 PRIVATE ws2_32
)

target_include_directories(automarket PUBLIC
                      "${PROJECT_SOURCE_DIR}/src"
                      "${PROJECT_SOURCE_DIR}/src/menu"
                      "${PROJECT_SOURCE_DIR}/src/include"
                      "${PROJECT_SOURCE_DIR}/src/include/directx"
)

target_compile_options(automarket PUBLIC -fpermissive -static -static-libgcc -static-libstdc++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")