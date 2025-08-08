cmake_minimum_required(VERSION 3.25)

set(AgentService_VERSION_NUMBER 1.0.0.0)
set(AgentService_VERSION_NAME "")
set(SRC_SUBDIRS agent cli config libs logging modules pipeline registration)
set(PLATFORM_SRC_DIR "")

# Top-level CMakeLists.txt
message(STATUS "CMAKE_SYSTEM_NAME is: ${CMAKE_SYSTEM_NAME}")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    file(GLOB PLATFORM_SRC_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux/*.cpp")
    file(GLOB PLATFORM_SRC_HPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux/*.hpp")
    set(PLATFORM_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/platform/linux")
# elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
#     set(PLATFORM_SRC
#         platform/windows/system_info.cpp
#         platform/windows/wg_setup.cpp
#     )
# elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
#     set(PLATFORM_SRC
#         platform/macos/system_info.cpp
#         platform/macos/wg_setup.cpp
#     )
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
##
##
add_executable(AgentService  ${PLATFORM_SRC})

##
##
##
target_sources(
  AgentService
  PRIVATE
    ${PLATFORM_SRC_CPP_FILES}
    ${PLATFORM_SRC_HPP_FILES}
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
    src
)

##
##
##
# target_link_libraries(AgentService PRIVATE spdlog::spdlog magic_enum cpr::cpr pthread)
target_link_libraries(AgentService PRIVATE spdlog::spdlog cpr::cpr pthread)
