/**
 * @file IKvsBackend.cpp
 * @brief Implementation of IKvsBackend static utility methods
 * @version 2.0
 * @date 2025-11-14
 */

#include "IKvsBackend.hpp"

namespace lap
{
namespace per
{
    void IKvsBackend::formatKey(core::String& key, EKvsDataTypeIndicate valueType)
    {
        // Check if key already has magic prefix
        if (key.size() >= 2 && key[DEF_KVS_MAGIC_KEY_INDEX] == DEF_KVS_MAGIC_KEY) {
#ifdef LAP_DEBUG
            LAP_PER_LOG_INFO << "Key is already formatted";
#endif
            return;
        }

        // Insert magic key prefix: "^" + type character
        // Type character: 'a' + type enum value
        key.insert(0, 2, static_cast<core::Char>(97 /*'a'*/ + static_cast<core::UInt32>(valueType)));
        key[DEF_KVS_MAGIC_KEY_INDEX] = DEF_KVS_MAGIC_KEY;
    }

    EKvsDataTypeIndicate IKvsBackend::getDataType(core::StringView strKey)
    {
        // Check if key has magic prefix
        if (strKey.size() < 2 || strKey[DEF_KVS_MAGIC_KEY_INDEX] != DEF_KVS_MAGIC_KEY) {
            // No prefix, assume string type
            return EKvsDataTypeIndicate::DataType_string;
        }

        // Extract type character
        core::Char cType = strKey[DEF_KVS_MAGIC_TYPE_INDEX];

        // Validate type character
        if (cType < 'a') {
            return EKvsDataTypeIndicate::DataType_string;
        }

        // Calculate enum value: type char - 'a'
        return static_cast<EKvsDataTypeIndicate>(cType - 'a');
    }

} // namespace per
} // namespace lap
