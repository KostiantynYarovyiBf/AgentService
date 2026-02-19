cmake_minimum_required(VERSION 3.25)

set(AgentService_VERSION_NUMBER 0.0.0.2)
set(AgentService_VERSION_NAME "")
set(SRC_SUBDIRS agent cli common config logging registration)
set(PLATFORM_SRC_DIR "")

message(STATUS "CMAKE_SYSTEM_NAME is: ${CMAKE_SYSTEM_NAME}")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    # TODO use this libs for setup network interfaces
    # find_package(PkgConfig REQUIRED)

    file(GLOB PLATFORM_COMMON_SRC_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/common/*.cpp")
    file(GLOB PLATFORM_SRC_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux/*.cpp")
    file(GLOB PLATFORM_SRC_HPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux/*.hpp")
    set(PLATFORM_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux")
    set(PLATFORM_COMMON_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/platform/common")

elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    # macOS platform configuration
    message(STATUS "Configuring for macOS")
    file(GLOB PLATFORM_COMMON_SRC_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/common/*.cpp")
    file(GLOB PLATFORM_SRC_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/macos/*.cpp")
    file(GLOB PLATFORM_SRC_HPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/macos/*.hpp")
    set(PLATFORM_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/platform/macos")
    set(PLATFORM_COMMON_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/platform/common")

# elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
endif()




project(AgentService VERSION ${AgentService_VERSION_NUMBER})

##
##
##
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##
##
##
set(SOURCES_CPP_FILES "")
set(SOURCES_HPP_FILES "")
set(SOURCES_C_FILES "")
set(SOURCES_H_FILES "")

##
##
##
foreach(subdir ${SRC_SUBDIRS})
    file(GLOB_RECURSE TMP_CPP "${CMAKE_CURRENT_LIST_DIR}/src/${subdir}/*.cpp")
    file(GLOB_RECURSE TMP_HPP "${CMAKE_CURRENT_LIST_DIR}/src/${subdir}/*.hpp")
    file(GLOB_RECURSE TMP_C   "${CMAKE_CURRENT_LIST_DIR}/src/${subdir}/*.c")
    file(GLOB_RECURSE TMP_H   "${CMAKE_CURRENT_LIST_DIR}/src/${subdir}/*.h")
    list(APPEND SOURCES_CPP_FILES ${TMP_CPP})
    list(APPEND SOURCES_HPP_FILES ${TMP_HPP})
    list(APPEND SOURCES_C_FILES   ${TMP_C})
    list(APPEND SOURCES_H_FILES   ${TMP_H})
endforeach()
list(APPEND SOURCES_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")

include(FetchContent)

##
## Use FetchContent to grab spdlog
##
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

##
## Use FetchContent to grab xxhr
##
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
set(CPR_BUILD_TESTS OFF)
set(CPR_USE_SYSTEM_CURL ON)
FetchContent_MakeAvailable(cpr)

##
## Find nlohmann-json
##
find_package(nlohmann_json REQUIRED)

##
## nlohmann_json
##
add_executable(AgentService)

##
##
##
target_sources(
  AgentService
  PRIVATE
    ${PLATFORM_SRC_CPP_FILES}
    ${PLATFORM_SRC_HPP_FILES}
    ${PLATFORM_COMMON_SRC_CPP_FILES}
    ${SOURCES_CPP_FILES}
    ${SOURCES_HPP_FILES}
    ${SOURCES_C_FILES}
    ${SOURCES_H_FILES}
)


##
##  
##
target_include_directories(AgentService PRIVATE
    ${PLATFORM_SRC_DIR} 
    ${PLATFORM_COMMON_SRC_DIR}   
    src
)

##
##  
##
add_subdirectory(third_party)

##
## Platform-specific libraries
##
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_link_libraries(AgentService 
        PRIVATE
        spdlog::spdlog
        cpr::cpr
        nlohmann_json::nlohmann_json
        pthread
        wireguard
    )
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    target_link_libraries(AgentService 
        PRIVATE
        spdlog::spdlog
        cpr::cpr
        nlohmann_json::nlohmann_json
        pthread
        # ndpi_wrapper #TODO undo when include this third part lib
        # macOS system frameworks
        "-framework CoreFoundation"
        "-framework SystemConfiguration"
        "-framework IOKit"
    )
else()
    # Default/fallback libraries
    target_link_libraries(AgentService 
        PRIVATE
        spdlog::spdlog
        cpr::cpr
        nlohmann_json::nlohmann_json
        pthread
    )
endif()


include(GNUInstallDirs)

# Install the main binary
install(TARGETS AgentService
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Optionally install config, docs, etc.
# install(FILES config.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/AgentService)
# install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Set package metadata
set(CPACK_PACKAGE_NAME "AgentService")
set(CPACK_PACKAGE_VERSION "${AgentService_VERSION_NUMBER}")
set(CPACK_PACKAGE_CONTACT "Kostiantyn yarovyi <kyarovyi@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kostiantyn yarovyi")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4-openssl-dev, libssl-dev, wireguard-tools, libmnl-dev, nlohmann-json3-dev, libnl-3-dev, libnl-route-3-dev, pkg-config")

# Enable DEB and TGZ generators
set(CPACK_GENERATOR "DEB;TGZ")

include(CPack)
