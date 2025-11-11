/**
 * @file CPersistency.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_PERSISTENCY_HPP
#define LAP_PERSISTENCY_PERSISTENCY_HPP

#include <lap/core/CCore.hpp>
#include <lap/log/CLog.hpp>

#define LAP_DEBUG

// per common
#include "CDataType.hpp"
#include "CPerErrorDomain.hpp"

// per client
// file operator
#include "CReadAccessor.hpp"
#include "CReadWriteAccessor.hpp"
#include "CFileStorage.hpp"
#include "CUpdate.hpp"

// kv
#include "CKeyValueStorage.hpp"

#include "CPersistencyManager.hpp"

#endif
