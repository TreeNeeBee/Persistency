/**
 * @file CUpdate.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_UPDATE_HPP
#define LAP_PERSISTENCY_UPDATE_HPP

#include "CDataType.hpp"

namespace lap
{
namespace per
{
    // Recovery report kinds for redundancy callbacks
    enum class RecoveryReportKind : core::UInt32 
    {
        kStorageLocationLost = 0,
        kRedundancyLost = 1,
        kStorageLocationRestored = 2,
        kRedundancyRestored = 3
    };

    // Update and Installation APIs
    void                        RegisterDataUpdateIndication( std::function<void()> callback ) noexcept;
    void                        RegisterApplicationDataUpdateCallback( std::function<void(const core::InstanceSpecifier&)> appDataUpdateCallback ) noexcept;
    core::Result<void>          UpdatePersistency() noexcept;
    core::Result<void>          CleanUpPersistency() noexcept;
    core::Result<void>          ResetPersistency() noexcept;
    core::Result<core::Bool>    CheckForManifestUpdate() noexcept;
    core::Result<void>          ReloadPersistencyManifest() noexcept;

    // Redundancy and Recovery APIs
    void                        RegisterRecoveryReportCallback( 
                                    std::function<void(
                                        const core::InstanceSpecifier&,
                                        RecoveryReportKind
                                    )> callback 
                                ) noexcept;
}
}

#endif
