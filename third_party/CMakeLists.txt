cmake_minimum_required(VERSION 3.25)


##
## Third_party common libs
##
file(GLOB_RECURSE NDPI_C CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/ndpi/*.c")
file(GLOB_RECURSE NDPI_H CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/ndpi/*.h")
file(GLOB_RECURSE NDPI_CPP CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/ndpi/*.cpp")
file(GLOB_RECURSE NDPI_HPP CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/ndpi/*.hpp")

##
##  nDPI - common, all platforms
##
# TODO uncommit when add this lib
# add_library(ndpi STATIC
# ${NDPI_C}
# ${NDPI_H}
# ${NDPI_CPP}
# ${NDPI_HPP}
# )
# target_include_directories(ndpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ndpi)

##
##  Third_party - Linux only
##
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    file(GLOB_RECURSE WG_C CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/wireguard/*.c")
    file(GLOB_RECURSE WG_H CONFIGURE_DEPENDS  "${CMAKE_CURRENT_LIST_DIR}/wireguard/*.h")
    add_library(wireguard STATIC
    ${WG_C}
    ${WG_H}
    )
    target_include_directories(wireguard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wireguard)
endif()
