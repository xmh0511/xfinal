cmake_minimum_required(VERSION 2.8)
set(project_name xfinal)
project(${project_name})
set(CMAKE_BUILD_TYPE Release)
SET(ENABLE_CLIENT_SSL OFF)
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -std=c++14")
endif ()

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "/usr/local/lib:")
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions(-DASIO_STANDALONE)

if(ENABLE_CLIENT_SSL)
	add_definitions(-DXFINAL_ENABLE_CLIENT_SSL)
endif()

if (ENABLE_CLIENT_SSL)
                find_package(OpenSSL REQUIRED)
endif()

set(XFINAL ./xfinal/xfinal.hpp main.cpp)

add_executable(${project_name} ${XFINAL})

if(ENABLE_CLIENT_SSL)
include_directories(${CMAKE_SOURCE_DIR}/xfinal ${CMAKE_SOURCE_DIR}/xfinal/asio ${CMAKE_SOURCE_DIR}/xfinal/ghc ${OPENSSL_INCLUDE_DIR})
else()
include_directories(${CMAKE_SOURCE_DIR}/xfinal ${CMAKE_SOURCE_DIR}/xfinal/asio ${CMAKE_SOURCE_DIR}/xfinal/ghc )
endif()


if(ENABLE_CLIENT_SSL)
target_link_libraries(${project_name} uuid ${OPENSSL_LIBRARIES})
else()
target_link_libraries(${project_name} uuid)
endif()

install(TARGETS ${project_name} DESTINATION include)
