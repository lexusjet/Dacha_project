cmake_minimum_required(VERSION 3.0.0)
project(DatabaseConnector CXX)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_CXX_STANDARD 20)

add_compile_options(
    
    # -Werror
    -Wall
    -Wno-comment
    -Wno-switch
    
    -Wextra
    -Wno-unused-parameter
    
    -Wpedantic
    -Wcast-align
    -Wcast-qual
    
    # -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    
    # -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    
    # -Wsign-conversion
    -Wsign-promo
    -Wno-unknown-pragmas
)

add_library(DatabaseConnectorLib STATIC
    src/DatabaseConnector.cpp
    include/DatabaseConnector/DatabaseConnector.h
)

target_link_libraries(DatabaseConnectorLib 
    PRIVATE mysqlcppconn
)

target_include_directories(DatabaseConnectorLib
    PUBLIC include
)