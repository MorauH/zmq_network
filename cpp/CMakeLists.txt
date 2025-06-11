cmake_minimum_required(VERSION 3.22.1)
project(zmq_network VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

# --- Dependencies ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(nlohmann_json REQUIRED)

# --- Variables ---
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# --- Build Library ---
add_library(zmq_network "${SRC_DIR}/pub.cc" "${SRC_DIR}/sub.cc")

# --- Set target properties ---
set_target_properties(zmq_network PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${INCLUDE_DIR}/zmq_network.hpp"
)

# --- Include directories ---
target_include_directories(zmq_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${SRC_DIR}
        ${ZMQ_INCLUDE_DIRS}
        ${INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(zmq_network 
    PUBLIC 
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# Compiler flags
target_compile_options(zmq_network PRIVATE ${ZMQ_CFLAGS_OTHER})

# --- Installation ---
include(GNUInstallDirs)

# Install the library
install(TARGETS zmq_network
    EXPORT zmq_network_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers if they exist
if(EXISTS "${INCLUDE_DIR}")
    install(DIRECTORY ${INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()


# --- Simple export (for find_package support) ---
install(EXPORT zmq_network_targets
    FILE zmq_networkTargets.cmake
    NAMESPACE zmq_network::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zmq_network
)
