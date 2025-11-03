#include <functional>
#include "CPerErrorDomain.hpp"
#include "CUpdate.hpp"

namespace lap
{
namespace pm
{
    void RegisterApplicationDataUpdateCallback ( ::std::function< void( const core::InstanceSpecifier &storage, core::String version )> ) noexcept
    {
        ;
    }

    core::Result<void> UpdatePersistency () noexcept
    {
        using result = core::Result<void>;

        return result::FromError( PerErrc::kPhysicalStorageFailure );
    }

    core::Result<void> ResetPersistency () noexcept
    {
        using result = core::Result<void>;

        return result::FromError( PerErrc::kPhysicalStorageFailure );
    }
} // pm
} // ara