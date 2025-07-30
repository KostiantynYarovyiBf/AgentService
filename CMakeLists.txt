cmake_minimum_required(VERSION 3.25)

set(AgentService_VERSION_NUMBER 1.0.0.0)
set(AgentService_VERSION_NAME "")

project(AgentService VERSION ${AgentService_VERSION_NUMBER})

##
##
##
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##
##
##
file(GLOB_RECURSE SOURCES_CPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB_RECURSE SOURCES_HPP_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp")


##
## Use FetchContent to grab spdlog
##
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

##
##
##
add_executable(AgentService)

##
##
##
target_sources(
  AgentService
  PRIVATE
    ${SOURCES_CPP_FILES}
    ${SOURCES_HPP_FILES}
)


##
##
##
target_include_directories(AgentService PRIVATE src)
target_link_libraries(AgentService PRIVATE spdlog::spdlog pthread)
