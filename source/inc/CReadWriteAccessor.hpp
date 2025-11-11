/**
 * @file CReadWriteAccessor.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_READWRITEACCESSOR_HPP
#define LAP_PERSISTENCY_READWRITEACCESSOR_HPP

#include "CReadAccessor.hpp"
#include "CFileStorage.hpp"

namespace lap
{
namespace per
{
    class ReadWriteAccessor : public ReadAccessor
    {
    public:
        IMP_OPERATOR_NEW(ReadWriteAccessor)

    public:
        explicit ReadWriteAccessor ( core::StringView, OpenMode, FileStorage* );
        virtual ~ReadWriteAccessor() noexcept;

        core::Result< void >            SyncToFile () noexcept;
        core::Result< void >            SetFileSize ( core::UInt64 size) noexcept;

        core::Result< void >            WriteText ( core::StringView s ) noexcept;
        core::Result< void >            WriteBinary ( core::Span< const core::Byte > b ) noexcept;

        ReadWriteAccessor&              operator<< ( core::StringView s ) noexcept;

        // template< typename...Args >
        // static auto Create(Args&&... args)  { return std::unique_ptr<ReadWriteAccessor> ( std::forward<Args>(args)... ); }

    protected:

    private:

    private:
    };
} // pm
} // ara

#endif
