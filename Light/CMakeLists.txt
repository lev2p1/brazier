# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.14)
project(lightlib VERSION 0.1.5 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    set(CMAKE_THREAD_PREFER_PTHREAD OFF)
    set(THREADS_PREFER_PTHREAD_FLAG OFF)
    set(CMAKE_USE_WIN32_THREADS_INIT ON)
    set(CMAKE_THREAD_LIBS_INIT "")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_PTHREADS_INIT 0)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(MSVC)
    add_compile_options(/utf-8 /bigobj)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS beast asio system thread filesystem)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

set(LIBRARIES
    Boost::beast
    Boost::asio
    Boost::system
    Boost::thread
    Boost::filesystem
    fmt::fmt
    nlohmann_json::nlohmann_json
    hiredis::hiredis
    ZLIB::ZLIB
    PostgreSQL::PostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
)

file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
    "src/Database/*.cpp"
    "src/Database/Migrations/*.cpp"
    "src/Router/*.cpp"
    "src/Filesystem/*.cpp"
    "src/Cryptography/*.cpp"
)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "app/Main\\.cpp$")

add_library(lightlib_objects OBJECT ${PROJECT_SOURCES})

target_link_libraries(lightlib_objects PUBLIC Boost::boost)

target_include_directories(lightlib_objects PRIVATE
    ${fmt_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${hiredis_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

target_include_directories(lightlib_objects PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(lightlib_objects PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/Database
    ${CMAKE_SOURCE_DIR}/Router
    ${CMAKE_SOURCE_DIR}/App
    ${CMAKE_SOURCE_DIR}/Storage
)
target_compile_features(lightlib_objects PUBLIC cxx_std_20)
target_compile_definitions(lightlib_objects PUBLIC
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_HAS_STD_COROUTINE
)

target_compile_features(lightlib_objects PUBLIC cxx_std_20)
target_compile_definitions(lightlib_objects PUBLIC
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_HAS_STD_COROUTINE
)

set_target_properties(lightlib_objects PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 8
)

add_library(lightlib_static STATIC $<TARGET_OBJECTS:lightlib_objects>)
add_library(lightlib_shared SHARED $<TARGET_OBJECTS:lightlib_objects>)

if(MSVC)
    set_target_properties(lightlib_shared PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

target_compile_definitions(lightlib_shared PRIVATE LIGHTLIB_EXPORTS)
target_compile_definitions(lightlib_shared PUBLIC LIGHTLIB_SHARED)
target_compile_definitions(lightlib_static PUBLIC)

set_target_properties(lightlib_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(lightlib_static PUBLIC ${LIBRARIES})
target_link_libraries(lightlib_shared PUBLIC ${LIBRARIES})

export(TARGETS lightlib_static lightlib_shared
    NAMESPACE lightlib::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/lightlibTargets.cmake"
)

add_executable(lightlib_app app/Main.cpp)

target_include_directories(lightlib_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/Database
    ${CMAKE_SOURCE_DIR}/Router
    ${CMAKE_SOURCE_DIR}/App
    ${CMAKE_SOURCE_DIR}/Storage
)

target_compile_features(lightlib_app PRIVATE cxx_std_20)

target_compile_definitions(lightlib_app PRIVATE
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_HAS_STD_COROUTINE
)

target_link_libraries(lightlib_app PRIVATE 
    lightlib_static
    ${LIBRARIES}
)

add_library(lightlib INTERFACE)
target_link_libraries(lightlib INTERFACE 
    lightlib_static
    lightlib_shared
)

install(TARGETS lightlib_static
    EXPORT lightlibTargets
    ARCHIVE DESTINATION lib
)

install(TARGETS lightlib_shared
    EXPORT lightlibTargets
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(TARGETS lightlib
    EXPORT lightlibTargets
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT lightlibTargets
    NAMESPACE lightlib::
    DESTINATION share/lightlib
    FILE lightlibTargets.cmake      
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lightlibConfig.cmake"
    DESTINATION share/lightlib
)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    message(STATUS "Building tests")
    
    find_package(GTest CONFIG REQUIRED)

    get_target_property(GTEST_INCLUDE_DIRS GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)

    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    add_executable(lightlib_tests ${TEST_SOURCES})

    target_include_directories(lightlib_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
        ${fmt_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
        ${hiredis_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
    )
    
    target_compile_definitions(lightlib_tests PRIVATE
        BOOST_ASIO_HAS_CO_AWAIT
        BOOST_ASIO_HAS_STD_COROUTINE
    )
    
    target_compile_features(lightlib_tests PRIVATE cxx_std_20)

    target_link_libraries(lightlib_tests PRIVATE
        lightlib_static
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        ${LIBRARIES}
    )

    # include(GoogleTest)
    # gtest_discover_tests(lightlib_tests)

    # add_test(NAME lightlib_tests COMMAND lightlib_tests)

    add_custom_target(check
        COMMAND $<TARGET_FILE:lightlib_tests>
        DEPENDS lightlib_tests
        COMMENT "Running tests..."
    )
    
    message(STATUS "Tests configured. Run 'cmake --build . --target check' to run tests")
endif()