add_library(getopt STATIC getopt_shim.cpp optparser.cpp)
target_include_directories(getopt INTERFACE ${CMAKE_CURRENT_LIST_DIR})
