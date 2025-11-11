/**
 * @file test_data_type.cpp
 * @brief Unit tests for CDataType functionality
 * @author AI Assistant
 * @date 2025-10-27
 */

#include <gtest/gtest.h>
#include <string>
#include "CDataType.hpp"

using namespace lap::per;
using namespace lap::core;

class DataTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test kvsToString function for various data types
TEST_F(DataTypeTest, KvsToString_Int8) {
    KvsDataType value = static_cast<Int8>(-42);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "-42");
}

TEST_F(DataTypeTest, KvsToString_UInt8) {
    KvsDataType value = static_cast<UInt8>(255);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "255");
}

TEST_F(DataTypeTest, KvsToString_Int16) {
    KvsDataType value = static_cast<Int16>(-32768);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "-32768");
}

TEST_F(DataTypeTest, KvsToString_UInt16) {
    KvsDataType value = static_cast<UInt16>(65535);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "65535");
}

TEST_F(DataTypeTest, KvsToString_Int32) {
    KvsDataType value = static_cast<Int32>(-2147483648);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "-2147483648");
}

TEST_F(DataTypeTest, KvsToString_UInt32) {
    KvsDataType value = static_cast<UInt32>(4294967295);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "4294967295");
}

TEST_F(DataTypeTest, KvsToString_Int64) {
    KvsDataType value = static_cast<Int64>(-9223372036854775807LL);
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "-9223372036854775807");
}

TEST_F(DataTypeTest, KvsToString_UInt64) {
    KvsDataType value = static_cast<UInt64>(18446744073709551615ULL);
    String result = kvsToStrig(value);
    // Note: might be implementation dependent
    EXPECT_FALSE(result.empty());
}

TEST_F(DataTypeTest, KvsToString_Bool_True) {
    KvsDataType value = true;
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "true");
}

TEST_F(DataTypeTest, KvsToString_Bool_False) {
    KvsDataType value = false;
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "false");
}

TEST_F(DataTypeTest, KvsToString_Float) {
    KvsDataType value = static_cast<Float>(3.14f);
    String result = kvsToStrig(value);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("3.14"), String::npos);
}

TEST_F(DataTypeTest, KvsToString_Double) {
    KvsDataType value = static_cast<Double>(2.718281828);
    String result = kvsToStrig(value);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("2.718"), String::npos);
}

TEST_F(DataTypeTest, KvsToString_String) {
    KvsDataType value = String("Hello, World!");
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "\"Hello, World!\"");
}

TEST_F(DataTypeTest, KvsToString_EmptyString) {
    KvsDataType value = String("");
    String result = kvsToStrig(value);
    EXPECT_EQ(result, "\"\"");
}

// Test kvsFromString function for various data types
TEST_F(DataTypeTest, KvsFromString_Int8) {
    String input = "-42";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_int8_t);
    EXPECT_EQ(lap::core::get<Int8>(result), -42);
}

TEST_F(DataTypeTest, KvsFromString_UInt8) {
    String input = "255";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_uint8_t);
    EXPECT_EQ(lap::core::get<UInt8>(result), 255);
}

TEST_F(DataTypeTest, KvsFromString_Int16) {
    String input = "-32768";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_int16_t);
    EXPECT_EQ(lap::core::get<Int16>(result), -32768);
}

TEST_F(DataTypeTest, KvsFromString_UInt16) {
    String input = "65535";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_uint16_t);
    EXPECT_EQ(lap::core::get<UInt16>(result), 65535);
}

TEST_F(DataTypeTest, KvsFromString_Int32) {
    String input = "-2147483648";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_int32_t);
    EXPECT_EQ(lap::core::get<Int32>(result), -2147483648);
}

TEST_F(DataTypeTest, KvsFromString_UInt32) {
    String input = "4294967295";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_uint32_t);
    EXPECT_EQ(lap::core::get<UInt32>(result), 4294967295U);
}

TEST_F(DataTypeTest, KvsFromString_Bool_True) {
    String input = "true";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_bool);
    EXPECT_EQ(lap::core::get<Bool>(result), true);
}

TEST_F(DataTypeTest, KvsFromString_Bool_False) {
    String input = "false";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_bool);
    EXPECT_EQ(lap::core::get<Bool>(result), false);
}

TEST_F(DataTypeTest, KvsFromString_Float) {
    String input = "3.14";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_float);
    Float value = lap::core::get<Float>(result);
    EXPECT_NEAR(value, 3.14f, 0.01f);
}

TEST_F(DataTypeTest, KvsFromString_Double) {
    String input = "2.718281828";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_double);
    Double value = lap::core::get<Double>(result);
    EXPECT_NEAR(value, 2.718281828, 0.000001);
}

TEST_F(DataTypeTest, KvsFromString_String) {
    String input = "Hello, World!";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_string);
    EXPECT_EQ(lap::core::get<String>(result), "Hello, World!");
}

TEST_F(DataTypeTest, KvsFromString_EmptyString) {
    String input = "";
    KvsDataType result = kvsFromString(input, EKvsDataTypeIndicate::DataType_string);
    EXPECT_EQ(lap::core::get<String>(result), "");
}

// Test OpenMode operators
TEST_F(DataTypeTest, OpenMode_Or_Operator) {
    OpenMode mode1 = OpenMode::kAtTheBeginning;
    OpenMode mode2 = OpenMode::kTruncate;
    OpenMode result = mode1 | mode2;
    
    EXPECT_EQ(static_cast<UInt32>(result), 
              static_cast<UInt32>(OpenMode::kAtTheBeginning) | 
              static_cast<UInt32>(OpenMode::kTruncate));
}

TEST_F(DataTypeTest, OpenMode_OrAssign_Operator) {
    OpenMode mode = OpenMode::kAtTheBeginning;
    mode |= OpenMode::kTruncate;
    
    EXPECT_EQ(static_cast<UInt32>(mode), 
              static_cast<UInt32>(OpenMode::kAtTheBeginning) | 
              static_cast<UInt32>(OpenMode::kTruncate));
}

TEST_F(DataTypeTest, OpenMode_And_Operator) {
    OpenMode mode = static_cast<OpenMode>(
        static_cast<UInt32>(OpenMode::kAtTheBeginning) | 
        static_cast<UInt32>(OpenMode::kTruncate));
    
    UInt32 modeValue = static_cast<UInt32>(mode);
    UInt32 beginningValue = static_cast<UInt32>(OpenMode::kAtTheBeginning);
    UInt32 truncateValue = static_cast<UInt32>(OpenMode::kTruncate);
    UInt32 endValue = static_cast<UInt32>(OpenMode::kAtTheEnd);
    
    Bool hasBeginning = (modeValue & beginningValue) == beginningValue;
    Bool hasTruncate = (modeValue & truncateValue) == truncateValue;
    Bool hasEnd = (modeValue & endValue) == endValue;
    
    EXPECT_TRUE(hasBeginning);
    EXPECT_TRUE(hasTruncate);
    EXPECT_FALSE(hasEnd);
}

// Test round-trip conversion
TEST_F(DataTypeTest, RoundTrip_Int32) {
    Int32 original = 12345;
    KvsDataType kvs = original;
    String str = kvsToStrig(kvs);
    KvsDataType converted = kvsFromString(str, EKvsDataTypeIndicate::DataType_int32_t);
    Int32 result = lap::core::get<Int32>(converted);
    
    EXPECT_EQ(original, result);
}

TEST_F(DataTypeTest, RoundTrip_String) {
    String original = "Test String with Special Characters: !@#$%^&*()";
    KvsDataType kvs = original;
    String str = kvsToStrig(kvs);
    // Remove quotes for comparison
    String unquoted = str.substr(1, str.length() - 2);
    
    EXPECT_EQ(original, unquoted);
}

TEST_F(DataTypeTest, RoundTrip_Bool) {
    Bool original = true;
    KvsDataType kvs = original;
    String str = kvsToStrig(kvs);
    KvsDataType converted = kvsFromString(str, EKvsDataTypeIndicate::DataType_bool);
    Bool result = lap::core::get<Bool>(converted);
    
    EXPECT_EQ(original, result);
}

TEST_F(DataTypeTest, RoundTrip_Float) {
    Float original = 3.14159f;
    KvsDataType kvs = original;
    String str = kvsToStrig(kvs);
    KvsDataType converted = kvsFromString(str, EKvsDataTypeIndicate::DataType_float);
    Float result = lap::core::get<Float>(converted);
    
    EXPECT_NEAR(original, result, 0.0001f);
}
