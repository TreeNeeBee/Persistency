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
namespace pm
{
    void                        RegisterApplicationDataUpdateCallback ( std::function< void( const core::InstanceSpecifier &storage, core::String version )> appDataUpdateCallback ) noexcept;
    core::Result<void>          UpdatePersistency () noexcept;
    core::Result<void>          ResetPersistency () noexcept;
}
}

#endif
