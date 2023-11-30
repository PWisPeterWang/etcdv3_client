cmake_minimum_required(VERSION 3.21)
set(ETCDV3_CLIENT_VERSION_MAJOR 0)
set(ETCDV3_CLIENT_VERSION_MINOR 0)
set(ETCDV3_CLIENT_VERSION_PATCH 1)
project(etcdv3_client
    LANGUAGES
        CXX C 
    VERSION
        ${ETCDV3_CLIENT_VERSION_MAJOR}.${ETCDV3_CLIENT_VERSION_MINOR}.${ETCDV3_CLIENT_VERSION_PATCH})
# edit the following settings as you desire
set(CMAKE_CXX_EXTENSIONS OFF)

# edit the following line to add your cmake modules
# if your don't build gRPC from source with cmake
# use the FindgRPC.cmake in cmake_modules
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

# edit the following line to add your dependencies
set(Boost_USE_STATIC_LIBS ON)
# my etcdv3-api-cpp project
# find_package(etcdv3-api-cpp CONFIG REQUIRED)
# find_package(Protobuf REQUIRED)
# find_package(gRPC REQUIRED)
add_subdirectory(thirdparty/etcdv3-api-cpp)
find_package(Threads REQUIRED)
# used boost.asio for steady timer, and tokenizer for parsing the urls
find_package(Boost REQUIRED COMPONENTS regex)
find_package(spdlog REQUIRED)

# Please note, CMake does not recommend GLOB to collect a list of source files from your source tree.
# Any new files added to your source tree won't be noticed by CMake until you rerun CMake manually.
file(GLOB_RECURSE LIBRARY_SRC "detail/*.cpp")
add_library(${PROJECT_NAME}
    STATIC
        ${LIBRARY_SRC})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/etcdv3_client>
        $<INSTALL_INTERFACE:etcdv3_client>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/detail
        ${Boost_INCLUDE_DIR})
# edit the following line to link your dependencies libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        Boost::regex
	# etcdv3-api-cpp::etcdv3-api-cpp
        etcdv3-api-cpp
        Threads::Threads)

# edit the cxx standard as you wish
target_compile_features(${PROJECT_NAME}
    PUBLIC
        cxx_std_17)

# this option saves a lot of time and brains
target_compile_options(${PROJECT_NAME}
    PUBLIC
        "-Wfatal-errors")

add_subdirectory(tests)
# install settings
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ETCDV3_CLIENT_EXPORT
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install public headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/etcdv3_client
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h")

install(EXPORT ETCDV3_CLIENT_EXPORT
    FILE etcdv3_client-config.cmake
    NAMESPACE etcdv3_client::
    DESTINATION cmake)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "etcdv3_client-config-version.cmake"
    COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/etcdv3_client-config-version.cmake"
    DESTINATION cmake)
message(STATUS "current etcdv3_client version: ${ETCDV3_CLIENT_VERSION_MAJOR}.${ETCDV3_CLIENT_VERSION_MINOR}.${ETCDV3_CLIENT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR ${ETCDV3_CLIENT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ETCDV3_CLIENT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ETCDV3_CLIENT_VERSION_PATCH})
# cpack settings, edit the following to pack up as you desire
if(UNIX)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "ZIP")
endif()
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/packages)
include(CPack)
