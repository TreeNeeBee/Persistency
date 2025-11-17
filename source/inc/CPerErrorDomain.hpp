/**
 * @file CPerErrorDomain.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_PERERRORDOMAIN_HPP
#define LAP_PERSISTENCY_PERERRORDOMAIN_HPP

#include <exception>
#include <cerrno>
#include <cstring>
#include <lap/core/CErrorCode.hpp>
#include <lap/core/CException.hpp>
#include <lap/core/CMemory.hpp>

#include "CDataType.hpp"

namespace lap
{
namespace per
{
    enum class PerErrc : core::ErrorDomain::CodeType 
    {
        kStorageNotFound            = 1,
        kKeyNotFound                = 2,
        kIllegalWriteAccess         = 3,
        kPhysicalStorageFailure     = 4,
        kIntegrityCorrupted         = 5,
        kValidationFailed           = 6,
        kEncryptionFailed           = 7,
        kDataTypeMismatch           = 8,
        kInitValueNotAvailable      = 9,
        kResourceBusy               = 10,
        kOutOfMemorySpace           = 11,     // NEW: AUTOSAR R22-11+
        kOutOfStorageSpace          = 12,
        kFileNotFound               = 13,
        kNotInitialized             = 14,
        kInvalidPosition            = 15,
        kIsEof                      = 16,
        kInvalidOpenMode            = 17,
        kInvalidSize                = 18,
        KPermissionDenied           = 19,
        kUnsupported                = 20,
        kWrongDataType              = 21,     // NEW: AUTOSAR type validation
        kWrongDataSize              = 22,     // NEW: AUTOSAR size validation
        kInvalidKey                 = 23,     // NEW: AUTOSAR key validation
        kInvalidArgument            = 24,     // NEW: Invalid argument provided
        kChecksumMismatch           = 25      // NEW: Checksum verification failed
    };

    inline constexpr const core::Char* PerErrMessage( PerErrc errCode )
    {
        auto const code = static_cast< PerErrc >( errCode );

        switch ( code ) {
        case PerErrc::kStorageNotFound:
            return "The passed InstanceSpecifier does not match any PersistencyKeyValueStorageInterface configured for this Executable.";
        case PerErrc::kKeyNotFound:
            return "The provided key cannot be not found in the Key-Value Storage.";
        case PerErrc::kIllegalWriteAccess:
            return "Opening a file for writing or changing, or synchronizing a key failed, because the storage is configured read-only.";
        case PerErrc::kPhysicalStorageFailure:
            return "Access to the storage fails.";
        case PerErrc::kIntegrityCorrupted:
            return "Stored data cannot be read because the structural integrity is corrupted.";
        case PerErrc::kValidationFailed:
            return "The validity of stored data cannot be ensured.";
        case PerErrc::kEncryptionFailed:
            return "The decryption of stored data fails.";
        case PerErrc::kDataTypeMismatch:
            return "The provided data type does not match the stored data type.";
        case PerErrc::kInitValueNotAvailable:
            return "The operation could not be performed because no initial value is available.";
        case PerErrc::kResourceBusy:
            return "UpdatePersistency or ResetPersistency is currently being executed, or if RecoverKeyValue Storage or ResetKeyValueStorage is currently being executed for the same Key-Value Storage.";
        case PerErrc::kOutOfMemorySpace:
            return "The available memory space is insufficient for the operation.";
        case PerErrc::kOutOfStorageSpace:
            return "The available storage space is insufficient for the added/updated values.";
        case PerErrc::kFileNotFound:
            return "The requested file cannot be not found in the File Storage.";
        case PerErrc::kNotInitialized:
            return "This function is called before lap::core::Initialize or after lap::core::Deinitialize.";
        case PerErrc::kInvalidPosition:
            return "SetPosition tried to move to a position that is not reachable (i.e. which is smaller than zero or greater than the current size of the file).";
        case PerErrc::kIsEof:
            return "The application tried to read from the end of the file or from an empty file.";
        case PerErrc::kInvalidOpenMode:
            return "Opening a file failed because the requested combination of OpenModes is invalid.";
        case PerErrc::kInvalidSize:
            return "SetFileSize tried to set a new size that is bigger than the current file size.";
        case PerErrc::KPermissionDenied:
            return std::strerror( EACCES );
        case PerErrc::kUnsupported:
            return "Not support yet.";
        case PerErrc::kWrongDataType:
            return "The data type provided does not match the expected type.";
        case PerErrc::kWrongDataSize:
            return "The data size provided does not match the expected size.";
        case PerErrc::kInvalidKey:
            return "The provided key is invalid or malformed.";
        case PerErrc::kInvalidArgument:
            return "Invalid argument provided to the function.";
        case PerErrc::kChecksumMismatch:
            return "Checksum verification failed - data integrity compromised.";
        default:
            return "Unknown error";
        }
    }

    class PerException : public core::Exception
    {
    public:
        IMP_OPERATOR_NEW(PerException)
        
        explicit PerException ( core::ErrorCode errorCode ) noexcept
            : core::Exception( errorCode )
        {
            ;
        }

        ~PerException() noexcept 
        {
            ;
        }

        const core::Char* what() const noexcept 
        {
            return PerErrMessage( static_cast< PerErrc > ( Error().Value() ) );
        }
    };

    class PerErrorDomain final : public core::ErrorDomain
    {
    public:
        IMP_OPERATOR_NEW(PerErrorDomain)
        
        using Errc          = PerErrc;
        using Exception     = PerException;

    public:
        const core::Char*                       Name () const noexcept override                                             { return "PerErrorDomain"; }
        const core::Char*                       Message ( CodeType errorCode ) const noexcept override                      { return PerErrMessage( static_cast< Errc >( errorCode ) ); }
        void                                    ThrowAsException ( const core::ErrorCode &errorCode ) const override        { throw PerException( errorCode ); }

        constexpr PerErrorDomain () noexcept
            : core::ErrorDomain( 0x8000000000000101 )
        {
            ;
        }
    };

    static constexpr PerErrorDomain g_perErrorDomain;
    
    constexpr const core::ErrorDomain& GetPerDomain () noexcept
    {
        return g_perErrorDomain;
    }

    constexpr core::ErrorCode MakeErrorCode ( PerErrc code, core::ErrorDomain::SupportDataType data ) noexcept
    {
        return { static_cast< core::ErrorDomain::CodeType >( code ), GetPerDomain(), data };
    }
} // pm
} // ara

#endif
