#pragma once

#include <string>
#include <sstream>

#include <gtest/gtest.h>

#include <lap/core/CCore.hpp>
#include "CPersistency.hpp"

::lap::core::SharedHandle< ::lap::pm::FileStorage > testFS;

TEST( FileStorage, OpenFileStorage )
{
    auto resut = ::lap::pm::OpenFileStorage( ::lap::core::InstanceSpecifier( "test" ), true );
    ASSERT_TRUE( resut.HasValue() );
    testFS = resut.Value();

    testFS->initialize( "", true );

    auto fs1 = testFS->OpenFileWriteOnly( "test_file_w" );
    auto fs2 = testFS->OpenFileWriteOnly( "test_file_append" );

    if ( !fs1.HasValue() ) std::cout << lap::pm::PerErrMessage( static_cast<lap::pm::PerErrc>( fs1.Error().Value() ) ) << std::endl;
    if ( !fs2.HasValue() ) std::cout << lap::pm::PerErrMessage( static_cast<lap::pm::PerErrc>( fs2.Error().Value() ) ) << std::endl;

    auto files = testFS->GetAllFileNames();
    EXPECT_TRUE( files.HasValue() );
    EXPECT_NE( ::std::find( files.Value().begin(), files.Value().end(), "test_file_w" ), files.Value().end() );
    EXPECT_NE( ::std::find( files.Value().begin(), files.Value().end(), "test_file_append" ), files.Value().end() );

    auto files2 = testFS->GetAllFileNames();
    EXPECT_TRUE( files2.HasValue() );
    EXPECT_FALSE( files2.Value().empty() );
}

TEST( FileStorage, OpenFileWriteOnly )
{
    auto resut_w1 = testFS->OpenFileWriteOnly( "test_file_w" );
    EXPECT_NE( nullptr, resut_w1.Value() );

    auto resut_w2 = testFS->OpenFileWriteOnly( "test_file_w" );
    //EXPECT_EQ( resut_w1.Value()->raw(), resut_w2.Value()->raw() );

    EXPECT_TRUE( resut_w2.Value()->WriteText( "test_write" ).HasValue() );
    EXPECT_TRUE( resut_w2.Value()->SyncToFile().HasValue() );

    EXPECT_FALSE( resut_w2.Value()->GetChar().HasValue() );
    EXPECT_FALSE( resut_w2.Value()->ReadText().HasValue() );
}

TEST( FileStorage, OpenFileReadOnly )
{
    auto resut_r1 = testFS->OpenFileReadOnly( "test_file_w" );
    EXPECT_TRUE( resut_r1.HasValue() );
    
    auto readText1 = resut_r1.Value()->ReadText();
    EXPECT_TRUE( readText1.HasValue() );
    EXPECT_TRUE( resut_r1.Value()->IsEof() );
    EXPECT_EQ( readText1.Value(), "test_write" );
}

TEST( FileStorage, OpenFileReadWrite )
{
    auto resut_rw = testFS->OpenFileReadWrite( "test_file_append", ::lap::pm::OpenMode::kTruncate );
    EXPECT_TRUE( resut_rw.HasValue() );
    EXPECT_NE( nullptr, resut_rw.Value() );

    EXPECT_TRUE( resut_rw.Value()->WriteText( "test_a1" ).HasValue() );
    EXPECT_TRUE( resut_rw.Value()->SyncToFile().HasValue() );

    EXPECT_TRUE( resut_rw.Value()->SetPosition( 0 ).HasValue() );
    auto readText1 = resut_rw.Value()->ReadText();
    EXPECT_TRUE( readText1.HasValue() );
    EXPECT_EQ( readText1.Value(), "test_a1" );
}

TEST( FileStorage, WriteAccessor )
{
    auto resut_a1 = testFS->OpenFileWriteOnly( "test_file_append", ::lap::pm::OpenMode::kTruncate );
    EXPECT_TRUE( resut_a1.HasValue() );
    EXPECT_NE( nullptr, resut_a1.Value() );

    EXPECT_TRUE( resut_a1.Value()->WriteText( "test_a1" ).HasValue() );
    EXPECT_TRUE( resut_a1.Value()->SyncToFile().HasValue() );

    auto resut_a2 = testFS->OpenFileWriteOnly( "test_file_append", ::lap::pm::OpenMode::kAppend );
    EXPECT_TRUE( resut_a2.HasValue() );

    EXPECT_TRUE( resut_a2.Value()->WriteText( "resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\" ).HasValue() );
    EXPECT_TRUE( resut_a2.Value()->WriteText( "\nresut_a3" ).HasValue() );
    EXPECT_TRUE( resut_a2.Value()->SyncToFile().HasValue() );
}

TEST( FileStorage, ReadAccessor )
{
    /* test_a1resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\ */
    // resut_a3
    auto resut_r1 = testFS->OpenFileReadOnly( "test_file_append" );
    EXPECT_EQ( resut_r1.Value()->PeekChar().Value(), 't' );
    EXPECT_EQ( resut_r1.Value()->GetChar().Value(), 't' );
    EXPECT_EQ( resut_r1.Value()->PeekChar().Value(), 'e' );
    EXPECT_EQ( resut_r1.Value()->ReadText().Value(), "est_a1resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\\nresut_a3" );
    EXPECT_TRUE( resut_r1.Value()->IsEof() );
    EXPECT_EQ( resut_r1.Value()->GetPosition(), 49 );
    EXPECT_FALSE( resut_r1.Value()->ReadText().HasValue() );

    EXPECT_TRUE( resut_r1.Value()->MovePosition( ::lap::pm::Origin::kBeginning, 0 ).HasValue() );
    EXPECT_FALSE( resut_r1.Value()->IsEof() );
    EXPECT_EQ( resut_r1.Value()->GetPosition(), 0 );
    EXPECT_EQ( resut_r1.Value()->ReadText( 5 ).Value(), "test_" );

    EXPECT_TRUE( resut_r1.Value()->SetPosition( 0 ).HasValue() );
    EXPECT_EQ( resut_r1.Value()->GetPosition(), 0 );
    EXPECT_EQ( resut_r1.Value()->ReadLine().Value(), "test_a1resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\" );
    EXPECT_EQ( resut_r1.Value()->ReadLine().Value(), "resut_a3" );
    EXPECT_TRUE( resut_r1.Value()->IsEof() );
    EXPECT_FALSE( resut_r1.Value()->ReadText().HasValue() );
}

TEST( FileStorage, ReadWriteAccessor )
{
    auto resut_a1 = testFS->OpenFileReadWrite( "test_file_append", ::lap::pm::OpenMode::kTruncate );
    EXPECT_TRUE( resut_a1.HasValue() );
    EXPECT_NE( nullptr, resut_a1.Value() );

    EXPECT_TRUE( resut_a1.Value()->WriteText( "test_a1" ).HasValue() );
    EXPECT_TRUE( resut_a1.Value()->SyncToFile().HasValue() );

    auto resut_a2 = testFS->OpenFileReadWrite( "test_file_append", ::lap::pm::OpenMode::kAppend );
    EXPECT_TRUE( resut_a2.HasValue() );

    EXPECT_TRUE( resut_a2.Value()->WriteText( "resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\" ).HasValue() );
    EXPECT_TRUE( resut_a2.Value()->WriteText( "\nresut_a3" ).HasValue() );
    EXPECT_TRUE( resut_a2.Value()->SyncToFile().HasValue() );

    EXPECT_TRUE( resut_a2.Value()->SetPosition( 0 ).HasValue() );
    auto readText1 = resut_a2.Value()->ReadText();
    EXPECT_TRUE( readText1.HasValue() );
    EXPECT_EQ( readText1.Value(), "test_a1resut_a2$#@!%^&*()<>[]{}\"\'.,;:~|\\\nresut_a3" );
}