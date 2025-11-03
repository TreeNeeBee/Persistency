cmake_minimum_required ( VERSION "3.13.2" )
include ( ../../BuildTemplate/Config.cmake.in )

project ( Persistency )

set ( MODULE_NAME "Persistency" )
set ( MODULE_VERNO 1.0.0 )

set ( MODULE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

set ( Boost_USE_STATIC_LIBS OFF )
set ( Boost_USE_MULTITHREADED ON )

find_package ( Boost REQUIRED COMPONENTS system filesystem thread regex )

set ( MODULE_SOURCE_CXX_DIR ${MODULE_ROOT_DIR}/source )

# Manually specify source files to exclude problematic daemon files
file ( GLOB_RECURSE LOCAL_LIB_CXX_HEADERS CONFIGURE_DEPENDS ${MODULE_SOURCE_CXX_DIR}/*.h* )
file ( GLOB_RECURSE LOCAL_LIB_CXX_SOURCES CONFIGURE_DEPENDS ${MODULE_SOURCE_CXX_DIR}/*.c* )

# Remove client files (they have their own executable)
list ( REMOVE_ITEM LOCAL_LIB_CXX_SOURCES 
    ${MODULE_SOURCE_CXX_DIR}/client/CPropertyClient.cpp
    ${MODULE_SOURCE_CXX_DIR}/client/property_main.cpp
    ${MODULE_SOURCE_CXX_DIR}/daemon/CDaemonManager.cpp
)

# Clear MODULE_SOURCE_CXX_DIR to prevent double GLOB in SharedLibrary.cmake.in
unset ( MODULE_SOURCE_CXX_DIR )

# Create symlinks for headers to match include structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/modules/Core/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/core SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/modules/LogAndTrace/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/log SYMBOLIC)

# Also provide `lap/` prefixed include layout used by some new files
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/lap)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/modules/Core/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/lap/core SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/modules/LogAndTrace/source/inc 
     ${CMAKE_CURRENT_BINARY_DIR}/include/lap/log SYMBOLIC)

# Add the include directory with symlinks
set ( MODULE_EXTERNAL_INCLUDE_DIR 
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# set ( ENABLE_BUILD_PROTOBUF ON CACHE BOOL "Build persistency protobuf" FORCE )
# set ( MODULE_PROTO_DIR ${MODULE_ROOT_DIR}/doc )
# include ( ../../BuildTemplate/Protobuf.cmake.in )

set ( MODULE_EXTERNAL_LIB pthread Boost::system Boost::filesystem Boost::thread Boost::regex rt sqlite3 ${PLATFORM_SYSTEM_TARGET}_core ${PLATFORM_SYSTEM_TARGET}_log ${LOCAL_PROTO_NAME} )
set ( ENABLE_BUILD_SHARED_LIBRARY ON CACHE BOOL "Build persistency shared library" FORCE )
set ( ENABLE_BUILD_WITH_PLATFORM_PREX ON )
include ( ../../BuildTemplate/SharedLibrary.cmake.in )

enable_testing()
find_package ( GTest REQUIRED )
include_directories( ${GTEST_INCLUDE_DIRS} )

# Enable unit tests
set ( MODULE_TEST_DIR ${MODULE_ROOT_DIR}/test )
set ( ENABLE_BUILD_TEST ON CACHE BOOL "Build persistency test" FORCE )
set ( ENABLE_BUILD_UNITTEST ON CACHE BOOL "Build persistency unit tests" FORCE )
set ( MODULE_EXTERNAL_TEST_LIB ${PLATFORM_SYSTEM_TARGET}_core ${PLATFORM_SYSTEM_TARGET}_log ${PLATFORM_SYSTEM_TARGET}_persistency Threads::Threads dlt sqlite3 Boost::filesystem Boost::regex GTest::GTest GTest::Main )

include ( ../../BuildTemplate/Test.cmake.in )

# Register tests with CTest
add_test(NAME persistency_tests COMMAND persistency_test)
