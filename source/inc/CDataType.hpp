/**
 * @file CDataType.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_DATATYPE_HPP
#define LAP_PERSISTENCY_DATATYPE_HPP

#include <sstream>

// core
#include <lap/core/CTypedef.hpp>
#include <lap/core/CString.hpp>
#include <lap/core/CVariant.hpp>
#include <lap/log/CLog.hpp>

// persistency common
#include "CPerErrorDomain.hpp"

namespace lap 
{
namespace per 
{
    #define LAP_PER_LOG_CONTEXT_ID       "PM"
    #define LAP_PER_LOG_CONTEXT_DESC     "PM log ctx"

    #define LAP_DEBUG

#ifdef LAP_DEBUG
    #define LAP_PER_LOG                  LAP_LOG( LAP_PER_LOG_CONTEXT_ID, LAP_PER_LOG_CONTEXT_DESC, ::lap::log::LogLevel::kVerbose )
    #define LAP_PER_LOG_VERBOSE          LAP_PER_LOG.LogVerbose().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_DEBUG            LAP_PER_LOG.LogDebug().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_INFO             LAP_PER_LOG.LogInfo().WithLocation( __FILE__, __LINE__ )
#else
    #define LAP_PER_LOG                  LAP_LOG( LAP_PER_LOG_CONTEXT_ID, LAP_PER_LOG_CONTEXT_DESC, ::lap::log::LogLevel::kWarn )
    #define LAP_PER_LOG_VERBOSE          LAP_PER_LOG.LogOff()
    #define LAP_PER_LOG_DEBUG            LAP_PER_LOG.LogOff()
    #define LAP_PER_LOG_INFO             LAP_PER_LOG.LogOff()
#endif
    #define LAP_PER_LOG_WARN             LAP_PER_LOG.LogWarn().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_ERROR            LAP_PER_LOG.LogError().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_FATAL            LAP_PER_LOG.LogFatal().WithLocation( __FILE__, __LINE__ )

    enum class OpenMode : core::UInt32
    {
        kAtTheBeginning = 1 << 0,
        kAtTheEnd       = 1 << 1,
        kTruncate       = 1 << 2,
        kAppend         = 1 << 3,
        kBinary         = 1 << 4,
        kIn             = 1 << 5,
        kOut            = 1 << 5,
        kEnd            = 1 << 16
    };

    constexpr OpenMode operator| ( OpenMode left, OpenMode right )
    {
        return static_cast< OpenMode > ( static_cast< typename std::underlying_type_t< OpenMode > >( left ) | 
                                        static_cast< typename std::underlying_type_t< OpenMode > >( right ) );
    }
    OpenMode& operator|= ( OpenMode &left, const OpenMode &right );

    constexpr typename std::underlying_type_t< OpenMode > operator& ( OpenMode left, OpenMode right )
    {
        return ( static_cast< typename std::underlying_type_t< OpenMode > >( left ) & 
                static_cast< typename std::underlying_type_t< OpenMode > >( right ) );
    }

    constexpr ::std::ios_base::openmode convert( OpenMode mode ) noexcept
    {
        ::std::ios_base::openmode modeBase{ ::std::ios_base::openmode::_S_ios_openmode_end };

        if ( mode & OpenMode::kAtTheBeginning ) {
            // no need do anything
        }

        if ( mode & OpenMode::kAtTheEnd )       modeBase |= ::std::ios_base::ate;
        if ( mode & OpenMode::kTruncate )       modeBase |= ::std::ios_base::trunc;
        if ( mode & OpenMode::kAppend )         modeBase |= ::std::ios_base::app;
        if ( mode & OpenMode::kBinary )         modeBase |= ::std::ios_base::binary;
        if ( mode & OpenMode::kIn )             modeBase |= ::std::ios_base::in;
        if ( mode & OpenMode::kOut )            modeBase |= ::std::ios_base::out;

        return modeBase;
    }

    constexpr OpenMode convert( ::std::ios_base::openmode mode ) noexcept
    {
        OpenMode modeBase{ OpenMode::kEnd };

        if ( mode & ::std::ios_base::ate )      modeBase |= OpenMode::kAtTheEnd;
        if ( mode & ::std::ios_base::trunc )    modeBase |= OpenMode::kTruncate;
        if ( mode & ::std::ios_base::app )      modeBase |= OpenMode::kAppend;
        if ( mode & ::std::ios_base::binary )   modeBase |= OpenMode::kBinary;
        if ( mode & ::std::ios_base::in )       modeBase |= OpenMode::kIn;
        if ( mode & ::std::ios_base::out )      modeBase |= OpenMode::kOut;

        return modeBase;
    }

    constexpr inline core::Bool valid( OpenMode mode ) noexcept
    {
        // false if 
        //      kAtTheBeginning combined with kAtTheEnd
        //      kTruncate combined with kAtTheEnd

        if ( ( mode & OpenMode::kAtTheEnd ) && ( ( mode & OpenMode::kAtTheBeginning ) || ( mode & OpenMode::kTruncate ) ) ) {
            return false;
        }

        return true;
    }

    enum class FileCreationState : core::UInt32
    {
        kCreatedDuringInstallion    = 1,
        kCreatedDuringUpdate        = 2,
        kCreatedDuringReset         = 3,
        kCreatedDuringRecovery      = 4,
        kCreatedByApplication       = 5
    };

    enum class FileModificationState : core::UInt32 
    {
        kModifiedDuringUpdate       = 2,
        kModifiedDuringReset        = 3,
        kModifiedDuringRecovery     = 4,
        kModifiedByApplication      = 5
    };

    enum class FileCRCType : core::UInt32 
    {
        None                    = 1,
        Header                  = 2,
        Total                   = 3,
    };

    enum class LevelUpdateStrategy : core::UInt32 
    {
        Delete                  = 2,
        KeepExisting            = 1,
        Overwrite               = 0,
    };

    enum class RedundancyStrategy : core::UInt32 
    {
        None                    = 1,
        Redundant               = 0,
        RedundantPerElement     = 2,
    };

    struct FileInfo
    {
        core::UInt64                    creationTime;
        core::UInt64                    modificationTime;
        core::UInt64                    accessTime;
        core::Size                      fileSize;
        FileCreationState               fileCreationState;
        FileModificationState           fileModificationState;
    };

    enum class Origin : core::UInt32 
    {
        kBeginning  = 0,
        kCurrent    = 1,
        kEnd        = 2
    };

    constexpr std::ios_base::seekdir convert( Origin pos ) noexcept
    {
        switch( pos ) {
        case Origin::kBeginning:
            return ::std::ios_base::beg;
        case Origin::kCurrent:
            return ::std::ios_base::cur;
        case Origin::kEnd:
            return ::std::ios_base::end;
        }
        return ::std::ios_base::beg;
    }

    using KvsDataType = core::Variant< \
                            core::Int8, core::UInt8, \
                            core::Int16, core::UInt16, \
                            core::Int32, core::UInt32, \
                            core::Int64, core::UInt64, \
                            core::Bool, \
                            core::Float, core::Double, \
                            core::String >;

    enum class EKvsDataTypeIndicate : core::UInt32
    { 
        DataType_int8_t         = 0,
        DataType_uint8_t        = 1,
        DataType_int16_t        = 2,
        DataType_uint16_t       = 3,
        DataType_int32_t        = 4,
        DataType_uint32_t       = 5,
        DataType_int64_t        = 6,
        DataType_uint64_t       = 7,
        DataType_bool           = 8,
        DataType_float          = 9,
        DataType_double         = 10,
        DataType_string         = 11,
    };

    constexpr core::Bool operator== ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) == static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator!= ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) != static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator== ( core::Int32 left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) == static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator!= ( core::Int32 left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) != static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator> ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) > static_cast< core::UInt32 >( right );
    }

    core::String kvsToStrig( const KvsDataType& value );
    KvsDataType kvsFromString( const core::String &value, const EKvsDataTypeIndicate &type );

    enum class KvsBackendType : core::UInt32 
    {
        kvsLocal            = 1 << 0,
        kvsRemote           = 1 << 1,
        kvsFile             = 1 << 16,
        kvsSqlite           = 1 << 17,
        kvsProperty         = 1 << 18
    };

    constexpr KvsBackendType operator| ( KvsBackendType left, KvsBackendType right )
    {
        return static_cast< KvsBackendType >( static_cast< typename std::underlying_type< KvsBackendType >::type >( left ) |
                                                static_cast< typename std::underlying_type< KvsBackendType >::type >( right ) );
    }

    constexpr typename std::underlying_type< KvsBackendType >::type operator& ( KvsBackendType left, KvsBackendType right )
    {
        return ( static_cast< typename std::underlying_type< KvsBackendType >::type >( left ) & 
                    static_cast< typename std::underlying_type< KvsBackendType >::type >( right ) );
    }
} // pm
} // ara

#endif
