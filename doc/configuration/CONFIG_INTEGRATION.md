# Persistency Configuration Integration

**Date**: 2025-11-14  
**Version**: 3.0

## Overview

Persistency module now integrates with Core's `ConfigManager` for centralized configuration management. The module uses the keyword `persistency` with an embedded `__metadata__` section.

## Configuration Structure

```json
{
  "persistency": {
    "__metadata__": {
      "contractVersion": "3.0.0",
      "deploymentVersion": "3.0.0",
      "manifestVersion": "1.0.0",
      "minimumSustainedSize": 1048576,
      "maximumAllowedSize": 104857600,
      "replicaCount": 3,
      "minValidReplicas": 2,
      "checksumType": "CRC32",
      "encryptionEnabled": false,
      "encryptionAlgorithm": "",
      "encryptionKeyId": ""
    }
  },
  "__metadata__": {
    "version": 1,
    "description": "LightAP Persistency Configuration",
    "crc": "auto-generated",
    "timestamp": "auto-generated",
    "hmac": "auto-generated"
  }
}
```

## Configuration Fields

### Module: `persistency`

The persistency module configuration is accessed via `ConfigManager::getInstance().getModuleConfigJson("persistency")`.

### Section: `persistency.__metadata__`

All persistency-specific metadata is stored in the `__metadata__` section within the persistency module:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `contractVersion` | String | "1.0.0" | AUTOSAR contract version |
| `deploymentVersion` | String | "1.0.0" | Deployment version |
| `manifestVersion` | String | "1.0.0" | Manifest version |
| `minimumSustainedSize` | UInt64 | 1048576 (1MB) | Minimum storage size in bytes |
| `maximumAllowedSize` | UInt64 | 104857600 (100MB) | Maximum storage size in bytes |
| `replicaCount` | UInt32 | 3 | Number of replicas (N in M-out-of-N) |
| `minValidReplicas` | UInt32 | 2 | Minimum valid replicas (M in M-out-of-N) |
| `checksumType` | String | "CRC32" | Checksum algorithm ("CRC32" or "SHA256") |
| `encryptionEnabled` | Boolean | false | Enable encryption |
| `encryptionAlgorithm` | String | "" | Encryption algorithm (e.g., "AES256") |
| `encryptionKeyId` | String | "" | Encryption key identifier |

## Usage Example

### C++ Code

```cpp
#include <lap/core/CConfig.hpp>
#include "CPersistency.hpp"

using namespace lap::core;
using namespace lap::per;

int main() {
    // Initialize Core system
    auto initResult = Initialize();
    if (!initResult.HasValue()) {
        return 1;
    }
    
    // Initialize ConfigManager with config file
    ConfigManager& config = ConfigManager::getInstance();
    auto result = config.initialize("/path/to/config.json", true);
    
    // Set persistency configuration
    nlohmann::json persistencyConfig;
    persistencyConfig["__metadata__"]["replicaCount"] = 5;
    persistencyConfig["__metadata__"]["minValidReplicas"] = 3;
    persistencyConfig["__metadata__"]["checksumType"] = "SHA256";
    persistencyConfig["__metadata__"]["maximumAllowedSize"] = 209715200; // 200MB
    
    config.setModuleConfigJson("persistency", persistencyConfig);
    
    // Open FileStorage - will automatically load configuration from ConfigManager
    auto fsResult = OpenFileStorage(InstanceSpecifier("app_data"), true);
    if (!fsResult.HasValue()) {
        return 1;
    }
    
    auto fs = fsResult.Value();
    // FileStorage is now configured with N=5, M=3, SHA256 checksums
    
    return 0;
}
```

### Configuration File Example

```json
{
  "persistency": {
    "__metadata__": {
      "contractVersion": "3.0.0",
      "deploymentVersion": "3.0.0",
      "manifestVersion": "1.0.0",
      "minimumSustainedSize": 2097152,
      "maximumAllowedSize": 209715200,
      "replicaCount": 5,
      "minValidReplicas": 3,
      "checksumType": "SHA256",
      "encryptionEnabled": true,
      "encryptionAlgorithm": "AES256",
      "encryptionKeyId": "key_001"
    }
  },
  "logging": {
    "level": "info",
    "output": "file"
  },
  "__metadata__": {
    "version": 1,
    "description": "LightAP System Configuration"
  }
}
```

## Implementation Details

### Configuration Loading Flow

1. **FileStorage::initialize()** is called
2. Default metadata values are set (N=3, M=2, CRC32)
3. ConfigManager is queried for `persistency` module configuration
4. If `persistency.__metadata__` exists, values override defaults
5. If ConfigManager is not initialized or configuration is missing, defaults are used
6. Configuration is passed to CFileStorageManager for initialization

### Error Handling

- If ConfigManager is not initialized: Uses default configuration
- If `persistency` module not found: Uses default configuration
- If `__metadata__` section missing: Uses default configuration
- If individual fields are missing: Uses default values for those fields
- All configuration loading errors are logged but do not fail initialization

### Backward Compatibility

- The `strConfig` parameter in `FileStorage::initialize()` is now deprecated
- Existing code continues to work - the parameter is ignored
- Configuration is now exclusively loaded from ConfigManager

## Benefits

✅ **Centralized Configuration**: All module configurations in one place  
✅ **Security**: Leverages ConfigManager's CRC32/timestamp/HMAC verification  
✅ **Version Control**: Built-in versioning and rollback support  
✅ **Runtime Updates**: Change configuration without code recompilation  
✅ **AUTOSAR Compliance**: Follows AUTOSAR SWS_PER configuration patterns  
✅ **Namespace Isolation**: Module-specific configuration under `persistency` key  

## Migration Guide

### Before (v2.x)

```cpp
// Configuration was hardcoded or passed as JSON string
auto fs = OpenFileStorage(InstanceSpecifier("app_data"), true);
```

### After (v3.0)

```cpp
// Configuration from ConfigManager
ConfigManager& config = ConfigManager::getInstance();
config.initialize("config.json", true);

// Set module configuration
nlohmann::json persistencyConfig;
persistencyConfig["__metadata__"]["replicaCount"] = 3;
config.setModuleConfigJson("persistency", persistencyConfig);

// Open storage - automatically uses configuration
auto fs = OpenFileStorage(InstanceSpecifier("app_data"), true);
```

## See Also

- Core Module: `modules/Core/source/inc/CConfig.hpp`
- Core Config Example: `modules/Core/test/examples/config_example.cpp`
- Persistency Manager: `modules/Persistency/source/inc/CFileStorageManager.hpp`
- AUTOSAR Specification: SWS_PER (Persistency)
