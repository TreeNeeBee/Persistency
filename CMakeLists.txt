cmake_minimum_required ( VERSION "3.10.2" )
include ( ../../BuildTemplate/Config.cmake.in )

project ( Persistency )

set ( MODULE_NAME "Persistency" )
set ( MODULE_VERNO 1.0.0 )

set ( Boost_USE_STATIC_LIBS OFF )
set ( Boost_USE_MULTITHREADED ON )

find_package ( Boost REQUIRED COMPONENTS system filesystem thread regex )
find_package ( GTest REQUIRED )

set ( MODULE_EXTERNAL_INCLUDE_DIR /usr/local/include )
set ( MODULE_EXTERNAL_LIB_DIR /usr/local/lib )

# Build Persistency against local Modules/Core and LogAndTrace outputs
# Create symlinks for headers to match include structure (lap/core and lap/log)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/lap)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/../Core/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/lap/core SYMBOLIC)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/../LogAndTrace/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/lap/log SYMBOLIC)

set ( CORE_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include )
set ( CORE_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR} )
list ( APPEND MODULE_EXTERNAL_INCLUDE_DIR ${CORE_INCLUDE_DIR} )
list ( APPEND MODULE_EXTERNAL_LIB_DIR ${CORE_LIB_DIR} )
set ( MODULE_EXTERNAL_LIB ${PLATFORM_SYSTEM_TARGET}_core ${PLATFORM_SYSTEM_TARGET}_log dlt sqlite3 Threads::Threads Boost::system Boost::filesystem Boost::thread Boost::regex rt ssl crypto )

set ( MODULE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set ( MODULE_SOURCE_CXX_DIR ${MODULE_ROOT_DIR}/source )
set ( ENABLE_BUILD_SHARED_LIBRARY ON CACHE BOOL "Build persistency shared library" FORCE )
set ( ENABLE_BUILD_WITH_PLATFORM_PREX ON )

# Enable SQLite backend support (sqlite3 already linked)
add_definitions(-DLAP_ENABLE_SQLITE)
add_definitions(-DUNIT_TEST)

include ( ../../BuildTemplate/SharedLibrary.cmake.in )

# Ensure the persistency library exports its symbols for consumers (tests, examples)
# Override global hidden visibility for this target only
set_target_properties(${PLATFORM_SYSTEM_TARGET}_persistency PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

# Unit tests
set ( MODULE_TEST_DIR ${MODULE_ROOT_DIR}/test )
set ( ENABLE_BUILD_TEST ON CACHE BOOL "Build persistency tests" FORCE )
set ( ENABLE_BUILD_UNITTEST ON CACHE BOOL "Build persistency unit tests" FORCE )
# Use GTest::GTest instead of GTest::Main since we have custom main function
set ( MODULE_EXTERNAL_TEST_LIB ${PLATFORM_SYSTEM_TARGET}_core ${PLATFORM_SYSTEM_TARGET}_log ${PLATFORM_SYSTEM_TARGET}_persistency Threads::Threads dlt sqlite3 Boost::filesystem Boost::regex GTest::GTest )

# Note: TestFS.hpp and TestKVS.hpp are deprecated in favor of test_file_storage.cpp and test_key_value_storage.cpp
# These header-only tests are excluded from compilation

include ( ../../BuildTemplate/Test.cmake.in )

# Register tests with CTest
add_test(NAME persistency_tests COMMAND persistency_test)

# Build examples
set ( ENABLE_MODULE_EXAMPLES ON )

if ( ENABLE_MODULE_EXAMPLES )
    set ( EXAMPLES_DIR ${MODULE_TEST_DIR}/examples )
    set ( BENCHMARK_DIR ${MODULE_TEST_DIR}/benchmark )
    
    set ( EXAMPLE_SOURCES 
        ${EXAMPLES_DIR}/sqlite_backend_demo.cpp
        ${EXAMPLES_DIR}/test_sqlite_basic.cpp
        ${EXAMPLES_DIR}/config_integration_example.cpp
        ${EXAMPLES_DIR}/property_backend_demo.cpp
        ${EXAMPLES_DIR}/backend_comparison_example.cpp
        ${EXAMPLES_DIR}/property_backend_config_example.cpp
        ${EXAMPLES_DIR}/config_driven_property_backend_example.cpp
        ${EXAMPLES_DIR}/multi_backend_usage_example.cpp
        ${EXAMPLES_DIR}/property_memory_only_example.cpp
        ${EXAMPLES_DIR}/test_kvs_none.cpp
    )
    
    set ( BENCHMARK_SOURCES
        ${BENCHMARK_DIR}/performance_benchmark.cpp
    )
    
    set ( EXAMPLE_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${LOCAL_LIB_INCLUDE_DIRS} )
    set ( EXAMPLE_LIB ${PLATFORM_SYSTEM_TARGET}_core ${PLATFORM_SYSTEM_TARGET}_log ${PLATFORM_SYSTEM_TARGET}_persistency Threads::Threads sqlite3 Boost::filesystem Boost::regex )
    
    include_directories ( ${SDK_INCLUDE_DIR} ${EXAMPLE_INCLUDE_DIRS} ${MODULE_EXTERNAL_INCLUDE_DIR} )
    link_directories ( ${SDK_LIB_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${MODULE_EXTERNAL_LIB_DIR} )
    
    foreach ( EXAMPLE_SRC ${EXAMPLE_SOURCES} )
        get_filename_component ( EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE )
        
        add_executable ( ${EXAMPLE_NAME} ${EXAMPLE_SRC} )
        target_link_libraries ( ${EXAMPLE_NAME} PRIVATE ${EXAMPLE_LIB} )
        
        message ( STATUS "Added example: ${EXAMPLE_NAME}" )
    endforeach ()
    
    foreach ( BENCH_SRC ${BENCHMARK_SOURCES} )
        get_filename_component ( BENCH_NAME ${BENCH_SRC} NAME_WE )
        
        add_executable ( ${BENCH_NAME} ${BENCH_SRC} )
        target_link_libraries ( ${BENCH_NAME} PRIVATE ${EXAMPLE_LIB} )
        
        message ( STATUS "Added benchmark: ${BENCH_NAME}" )
    endforeach ()
endif ()
