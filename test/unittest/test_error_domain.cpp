/**
 * @file test_error_domain.cpp
 * @brief Unit tests for Persistency Error Domain
 * @author AI Assistant
 * @date 2025-10-27
 */

#include <gtest/gtest.h>
#include "CPerErrorDomain.hpp"

using namespace lap::pm;
using namespace lap::core;

class ErrorDomainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test error code enum
TEST_F(ErrorDomainTest, ErrorCode_Values) {
    EXPECT_EQ(static_cast<int>(PerErrc::kStorageNotFound), 1);
    EXPECT_EQ(static_cast<int>(PerErrc::kKeyNotFound), 2);
    EXPECT_EQ(static_cast<int>(PerErrc::kIllegalWriteAccess), 3);
    EXPECT_EQ(static_cast<int>(PerErrc::kPhysicalStorageFailure), 4);
    EXPECT_EQ(static_cast<int>(PerErrc::kIntegrityCorrupted), 5);
    EXPECT_EQ(static_cast<int>(PerErrc::kValidationFailed), 6);
    EXPECT_EQ(static_cast<int>(PerErrc::kEncryptionFailed), 7);
    EXPECT_EQ(static_cast<int>(PerErrc::kDataTypeMismatch), 8);
    EXPECT_EQ(static_cast<int>(PerErrc::kInitValueNotAvailable), 9);
    EXPECT_EQ(static_cast<int>(PerErrc::kResourceBusy), 10);
    EXPECT_EQ(static_cast<int>(PerErrc::kOutOfStorageSpace), 12);
    EXPECT_EQ(static_cast<int>(PerErrc::kFileNotFound), 13);
}

// Test error messages
TEST_F(ErrorDomainTest, ErrorMessage_PhysicalStorageFailure) {
    String msg = PerErrMessage(PerErrc::kPhysicalStorageFailure);
    EXPECT_FALSE(msg.empty());
}

TEST_F(ErrorDomainTest, ErrorMessage_IntegrityCorrupted) {
    String msg = PerErrMessage(PerErrc::kIntegrityCorrupted);
    EXPECT_FALSE(msg.empty());
}

TEST_F(ErrorDomainTest, ErrorMessage_ValidationFailed) {
    String msg = PerErrMessage(PerErrc::kValidationFailed);
    EXPECT_FALSE(msg.empty());
}

TEST_F(ErrorDomainTest, ErrorMessage_KeyNotFound) {
    String msg = PerErrMessage(PerErrc::kKeyNotFound);
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("Key"), String::npos);
}

TEST_F(ErrorDomainTest, ErrorMessage_OutOfStorageSpace) {
    String msg = PerErrMessage(PerErrc::kOutOfStorageSpace);
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("storage"), String::npos);
}

TEST_F(ErrorDomainTest, ErrorMessage_AllCodes) {
    // Test that all error codes have non-empty messages
    auto testCode = [](PerErrc code) {
        String msg = PerErrMessage(code);
        EXPECT_FALSE(msg.empty()) << "Error code " << static_cast<int>(code) << " has empty message";
    };

    testCode(PerErrc::kStorageNotFound);
    testCode(PerErrc::kPhysicalStorageFailure);
    testCode(PerErrc::kIntegrityCorrupted);
    testCode(PerErrc::kValidationFailed);
    testCode(PerErrc::kEncryptionFailed);
    testCode(PerErrc::kKeyNotFound);
    testCode(PerErrc::kIllegalWriteAccess);
    testCode(PerErrc::kOutOfStorageSpace);
    testCode(PerErrc::kResourceBusy);
    testCode(PerErrc::kInitValueNotAvailable);
    testCode(PerErrc::kDataTypeMismatch);
    testCode(PerErrc::kFileNotFound);
    testCode(PerErrc::kNotInitialized);
    testCode(PerErrc::kInvalidPosition);
    testCode(PerErrc::kIsEof);
    testCode(PerErrc::kInvalidOpenMode);
    testCode(PerErrc::kInvalidSize);
    testCode(PerErrc::kUnsupported);
}

// Test PerException
TEST_F(ErrorDomainTest, PerException_Construction) {
    ErrorCode errCode = MakeErrorCode(PerErrc::kKeyNotFound, 0);
    PerException ex(errCode);
    
    EXPECT_EQ(ex.Error(), errCode);
}

TEST_F(ErrorDomainTest, PerException_What) {
    ErrorCode errCode = MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0);
    PerException ex(errCode);
    
    const char* msg = ex.what();
    EXPECT_NE(msg, nullptr);
    EXPECT_TRUE(strlen(msg) > 0);
}

// Test MakeErrorCode function
TEST_F(ErrorDomainTest, MakeErrorCode) {
    ErrorCode ec1 = MakeErrorCode(PerErrc::kKeyNotFound, 0);
    EXPECT_EQ(ec1.Value(), static_cast<Int32>(PerErrc::kKeyNotFound));
    
    ErrorCode ec2 = MakeErrorCode(PerErrc::kOutOfStorageSpace, 0);
    EXPECT_EQ(ec2.Value(), static_cast<Int32>(PerErrc::kOutOfStorageSpace));
}

// Test different error codes are different
TEST_F(ErrorDomainTest, ErrorCode_Uniqueness) {
    ErrorCode ec1 = MakeErrorCode(PerErrc::kKeyNotFound, 0);
    ErrorCode ec2 = MakeErrorCode(PerErrc::kOutOfStorageSpace, 0);
    ErrorCode ec3 = MakeErrorCode(PerErrc::kKeyNotFound, 0);
    
    EXPECT_NE(ec1.Value(), ec2.Value());
    EXPECT_EQ(ec1.Value(), ec3.Value());
}
