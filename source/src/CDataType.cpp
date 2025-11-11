#include "CDataType.hpp"

namespace lap 
{
namespace per 
{
    OpenMode& operator|= ( OpenMode &left, const OpenMode &right )
    {
        left = left | right;

        return left;
    }

    core::String kvsToStrig( const KvsDataType& value )
    {
#if __cplusplus >= 201703L
        // C++17: std::variant uses index() instead of which()
        switch( static_cast< EKvsDataTypeIndicate >( value.index() ) ) {
        case EKvsDataTypeIndicate::DataType_int8_t: //Int8
            return ::std::to_string( ::std::get< core::Int8 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint8_t: // UInt8
            return ::std::to_string( ::std::get< core::UInt8 >( value ) );
        case EKvsDataTypeIndicate::DataType_int16_t: // Int16
            return ::std::to_string( ::std::get< core::Int16 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint16_t: // UInt16
            return ::std::to_string( ::std::get< core::UInt16 >( value ) );
        case EKvsDataTypeIndicate::DataType_int32_t: // Int32
            return ::std::to_string( ::std::get< core::Int32 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint32_t: // UInt32
            return ::std::to_string( ::std::get< core::UInt32 >( value ) );
        case EKvsDataTypeIndicate::DataType_int64_t: // Int64
            return ::std::to_string( ::std::get< core::Int64 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint64_t: // UInt64
            return ::std::to_string( ::std::get< core::UInt64 >( value ) );
        case EKvsDataTypeIndicate::DataType_bool: // Bool
            return ::std::get< core::Bool >( value ) ? "true" : "false";
        case EKvsDataTypeIndicate::DataType_float: // Float
            return ::std::to_string( ::std::get< core::Float >( value ) );
        case EKvsDataTypeIndicate::DataType_double: // Double
            return ::std::to_string( ::std::get< core::Double >( value ) );
        case EKvsDataTypeIndicate::DataType_string: // String
            return "\"" + ::std::get< core::String >( value ) + "\"";
        }
#else
        // C++14: boost::variant uses which()
        switch( static_cast< EKvsDataTypeIndicate >( value.which() ) ) {
        case EKvsDataTypeIndicate::DataType_int8_t: //Int8
            return ::std::to_string( ::boost::get< core::Int8 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint8_t: // UInt8
            return ::std::to_string( ::boost::get< core::UInt8 >( value ) );
        case EKvsDataTypeIndicate::DataType_int16_t: // Int16
            return ::std::to_string( ::boost::get< core::Int16 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint16_t: // UInt16
            return ::std::to_string( ::boost::get< core::UInt16 >( value ) );
        case EKvsDataTypeIndicate::DataType_int32_t: // Int32
            return ::std::to_string( ::boost::get< core::Int32 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint32_t: // UInt32
            return ::std::to_string( ::boost::get< core::UInt32 >( value ) );
        case EKvsDataTypeIndicate::DataType_int64_t: // Int64
            return ::std::to_string( ::boost::get< core::Int64 >( value ) );
        case EKvsDataTypeIndicate::DataType_uint64_t: // UInt64
            return ::std::to_string( ::boost::get< core::UInt64 >( value ) );
        case EKvsDataTypeIndicate::DataType_bool: // Bool
            return ::boost::get< core::Bool >( value ) ? "true" : "false";
        case EKvsDataTypeIndicate::DataType_float: // Float
            return ::std::to_string( ::boost::get< core::Float >( value ) );
        case EKvsDataTypeIndicate::DataType_double: // Double
            return ::std::to_string( ::boost::get< core::Double >( value ) );
        case EKvsDataTypeIndicate::DataType_string: // String
            return "\"" + ::boost::get< core::String >( value ) + "\"";
        }
#endif

        return "";
    }

    KvsDataType kvsFromString( const core::String &value, const EKvsDataTypeIndicate &type )
    {
        switch( type ) {
        case EKvsDataTypeIndicate::DataType_int8_t: //Int8
            return static_cast< core::Int8 >( ::std::stoi( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_uint8_t: // UInt8
            return static_cast< core::UInt8 >( ::std::stoi( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_int16_t: // Int16
            return static_cast< core::Int16 >( ::std::stoi( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_uint16_t: // UInt16
            return static_cast< core::UInt16 >( ::std::stoi( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_int32_t: // Int32
            return static_cast< core::Int32 >( ::std::stoi( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_uint32_t: // UInt32
            return static_cast< core::UInt32 >( ::std::stoul( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_int64_t: // Int64
            return static_cast< core::Int64 >( ::std::stoll( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_uint64_t: // UInt64
            return static_cast< core::UInt64 >( ::std::stoull( value.c_str() ) );
        case EKvsDataTypeIndicate::DataType_bool: // Bool
            return ( value == "true" );
        case EKvsDataTypeIndicate::DataType_float: // Float
            return ::std::stof( value.c_str() );
        case EKvsDataTypeIndicate::DataType_double: // Double
            return ::std::stod( value.c_str() );
        case EKvsDataTypeIndicate::DataType_string: // String
            return core::String( value.c_str() );
        }

        return "";
    }
} // pm
} // ara
