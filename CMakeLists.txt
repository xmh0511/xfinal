cmake_minimum_required(VERSION 2.8)
set(project_name xfinal)
project(${project_name})
set(CMAKE_BUILD_TYPE Release)
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
	add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -std=c++14")
endif ()

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "/usr/local/lib:")
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions(-DASIO_STANDALONE)

set(XFINAL main.cpp)

add_executable(${project_name} ${XFINAL})

include_directories(${CMAKE_SOURCE_DIR}/asio ${CMAKE_SOURCE_DIR}/ghc)


target_link_libraries(${project_name} uuid)

install(TARGETS ${project_name} DESTINATION include)
