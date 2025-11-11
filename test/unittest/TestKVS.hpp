#pragma once

#include <string>
#include <sstream>
#include <iostream>

#include <gtest/gtest.h>

#include <lap/core/CCore.hpp>
#include "CPersistency.hpp"

::lap::core::SharedHandle< ::lap::per::KeyValueStorage > testKVS;

TEST( KeyValueStorage, OpenKeyValueStorage )
{
    auto resut = ::lap::per::OpenKeyValueStorage( ::lap::core::InstanceSpecifier( "/tmp/test" ) );
    ASSERT_TRUE( resut.HasValue() );

    testKVS = resut.Value();
}

TEST( KeyValueStorage, KeyValueStorage )
{
    EXPECT_TRUE( testKVS->RemoveAllKeys().HasValue() );
    EXPECT_TRUE( testKVS->GetAllKeys().HasValue() );
    EXPECT_TRUE( testKVS->GetAllKeys().Value().empty() );
    EXPECT_FALSE( testKVS->KeyExists( "test" ).Value() );

    EXPECT_TRUE( testKVS->SetValue<lap::core::String>( "test1", "demo1" ).HasValue() );
    EXPECT_EQ( testKVS->GetAllKeys().Value().size(), 1 );
    EXPECT_TRUE( testKVS->KeyExists( "test1" ).Value() );

    EXPECT_TRUE( testKVS->SetValue<lap::core::String>( "test2", "demo2" ).HasValue() );
    EXPECT_TRUE( testKVS->KeyExists( "test2" ).Value() );
    EXPECT_TRUE( testKVS->SetValue<lap::core::String>( "tes1111111t2", "demo211111" ).HasValue() );
    EXPECT_EQ( testKVS->GetAllKeys().Value().size(), 3 );

    EXPECT_TRUE( testKVS->SyncToStorage().HasValue() );

    EXPECT_TRUE( testKVS->RemoveKey( "test" ).HasValue() );
    EXPECT_TRUE( testKVS->RemoveKey( "test1" ).HasValue() );
    EXPECT_FALSE( testKVS->KeyExists( "test1" ).Value() );
    EXPECT_TRUE( testKVS->KeyExists( "test2" ).Value() );

    EXPECT_TRUE( testKVS->RemoveAllKeys().HasValue() );
    EXPECT_FALSE( testKVS->KeyExists( "test2" ).Value() );

    EXPECT_TRUE( testKVS->DiscardPendingChanges().HasValue() );
    //EXPECT_EQ( testKVS->GetAllKeys().Value().size(), 2 );     // maybe false after DiscardPendingChanges when use property
}

TEST( KeyValueStorage, SetValue )
{
    EXPECT_TRUE( testKVS->RemoveAllKeys().HasValue() );

    EXPECT_TRUE( testKVS->SetValue( "int8_t", static_cast< ::lap::core::Int8 >(0) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "uint8_t", static_cast< ::lap::core::UInt8 >(1) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "int16_t", static_cast< ::lap::core::Int16 >(2) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "uint16_t", static_cast< ::lap::core::UInt16 >(3) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "int32_t", static_cast< ::lap::core::Int32 >(4) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "uint32_t", static_cast< ::lap::core::UInt32 >(5) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "int64_t", static_cast< ::lap::core::Int64 >(6) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "uint64_t", static_cast< ::lap::core::UInt64 >(7) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "bool", true ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "float", static_cast< ::lap::core::Float >(0.0) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "double", static_cast< ::lap::core::Double >(0.0) ).HasValue() );
    EXPECT_TRUE( testKVS->SetValue( "string", ::lap::core::String( "demo" ) ).HasValue() );

    // EXPECT_TRUE( testKVS->RemoveAllKeys().HasValue() );
    // // 2114 kvs for [64,256] in 1M
    // ::lap::core::Char buffer[65];
    // for ( uint i = 0; i < 10000; ++i ) {
    //     std::snprintf(buffer, 65, "12345678901234567890123456789012345678901234567890123456789^%.4d", i );
    //     std::string str( buffer );
    //     EXPECT_TRUE( testKVS->SetValue( str, ::lap::core::String( "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456" ) ).HasValue() );
    // }

    EXPECT_TRUE( testKVS->SyncToStorage().HasValue() );
    EXPECT_EQ( testKVS->GetAllKeys().Value().size(), 12 );
}

TEST( KeyValueStorage, GetValue )
{
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Int8 >( "int8_t" ).Value(), 0 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::UInt8 >( "uint8_t" ).Value(), 1 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Int16 >( "int16_t" ).Value(), 2 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::UInt16 >( "uint16_t" ).Value(), 3 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Int32 >( "int32_t" ).Value(), 4 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::UInt32 >( "uint32_t" ).Value(), 5 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Int64 >( "int64_t" ).Value(), 6 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::UInt64 >( "uint64_t" ).Value(), 7 );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Bool >( "bool" ).Value(), true );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Float >( "float" ).Value(), static_cast< ::lap::core::Float >(0.0) );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::Double >( "double" ).Value(), static_cast< ::lap::core::Double >(0.0) );
    EXPECT_EQ( testKVS->GetValue< ::lap::core::String >( "string" ).Value(), ::lap::core::String( "demo" ) );
}

::lap::core::SharedHandle< ::lap::per::KeyValueStorage > testRemoteKVS;

TEST( KeyValueStorage, OpenKeyValueStorageRemote )
{
#ifdef LAP_DEBUG
    auto resut = ::lap::per::OpenKeyValueStorage( ::lap::core::InstanceSpecifier( "global" ), true, ::lap::per::KvsBackendType::kvsFile );
#else
    auto resut = ::lap::per::OpenKeyValueStorage( ::lap::core::InstanceSpecifier( "global" ) );
#endif
    ASSERT_TRUE( resut.HasValue() );

    testRemoteKVS = resut.Value();
}

TEST( KeyValueStorageRemote, SetValue )
{
    EXPECT_TRUE( testRemoteKVS->SetValue( "int8_t", static_cast< ::lap::core::Int8 >(0) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "uint8_t", static_cast< ::lap::core::UInt8 >(1) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "int16_t", static_cast< ::lap::core::Int16 >(2) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "uint16_t", static_cast< ::lap::core::UInt16 >(3) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "int32_t", static_cast< ::lap::core::Int32 >(4) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "uint32_t", static_cast< ::lap::core::UInt32 >(5) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "int64_t", static_cast< ::lap::core::Int64 >(6) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "uint64_t", static_cast< ::lap::core::UInt64 >(7) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "bool", true ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "float", static_cast< ::lap::core::Float >(0.0) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "double", static_cast< ::lap::core::Double >(0.0) ).HasValue() );
    EXPECT_TRUE( testRemoteKVS->SetValue( "string", ::lap::core::String( "demo" ) ).HasValue() );

    EXPECT_EQ( testRemoteKVS->GetAllKeys().Value().size(), 12 );
}

TEST( KeyValueStorageRemote, GetValue )
{
    auto keys = testRemoteKVS->GetAllKeys().Value();
    for ( auto&& it = keys.begin(); it != keys.end(); ++it ) {
        ::std::cerr << "keys: " << *it << ::std::endl;
    }

    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Int8 >( "int8_t" ).Value(), 0 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::UInt8 >( "uint8_t" ).Value(), 1 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Int16 >( "int16_t" ).Value(), 2 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::UInt16 >( "uint16_t" ).Value(), 3 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Int32 >( "int32_t" ).Value(), 4 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::UInt32 >( "uint32_t" ).Value(), 5 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Int64 >( "int64_t" ).Value(), 6 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::UInt64 >( "uint64_t" ).Value(), 7 );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Bool >( "bool" ).Value(), true );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Float >( "float" ).Value(), static_cast< ::lap::core::Float >(0.0) );
    EXPECT_EQ( testRemoteKVS->GetValue< ::lap::core::Double >( "double" ).Value(), static_cast< ::lap::core::Double >(0.0) );
    EXPECT_EQ( testRemoteKVS->GetValue<lap::core::String>( "string" ).Value(), ::lap::core::String( "demo" ) );
}