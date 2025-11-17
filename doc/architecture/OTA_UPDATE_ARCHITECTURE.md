# AUTOSAR Persistency OTA 更新架构详解

**文档版本:** 1.0  
**AUTOSAR 标准:** AP R24-11  
**参考文档:** AUTOSAR_AP_SWS_Persistency (Document ID 858)  
**日期:** 2025-11-11

---

## 1. Update/Installation 的核心作用

### 1.1 目的

Persistency 的 Update/Installation 机制是 **AUTOSAR Adaptive Platform 软件生命周期管理** 的关键组成部分，主要负责：

1. **持久化数据与应用程序同步更新**
   - 当 Adaptive Application 更新时，其持久化数据也需要相应更新
   - 确保数据格式、类型、结构与新版本应用程序兼容

2. **数据迁移和转换**
   - 支持跨版本的数据类型转换（如 uint8 → uint16）
   - 支持数据单位转换（如 mph → km/h）
   - 支持数据结构变更（添加/删除字段）

3. **安全回滚机制**
   - 更新失败时自动恢复到之前的稳定版本
   - 保证数据一致性和系统稳定性

4. **配置独立更新**
   - 支持仅更新配置数据而不更新应用程序
   - 支持运行时检测配置更新并重新加载

---

## 2. OTA 更新的生命周期阶段

根据 AUTOSAR 标准，Update and Configuration Management (UCM) 处理 Adaptive Application 的以下生命周期阶段：

### 2.1 五个主要阶段

```
┌─────────────────────────────────────────────────────────────┐
│                    OTA 生命周期阶段                           │
├─────────────────────────────────────────────────────────────┤
│ 1. Installation    (安装新软件)                              │
│ 2. Update          (更新已安装的软件)                         │
│ 3. Finalization    (更新成功后的完成操作)                     │
│ 4. Roll-back       (更新失败后的回滚)                         │
│ 5. Removal         (删除已安装的软件)                         │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. OTA 更新流程架构

### 3.1 完整的 OTA 更新序列图

```
┌──────────────┐  ┌──────────────┐  ┌─────────────┐  ┌──────────────┐
│   UCM        │  │  Adaptive    │  │ Persistency │  │   Storage    │
│  (Update &   │  │ Application  │  │             │  │   Backend    │
│Config Mgmt)  │  │              │  │             │  │              │
└──────┬───────┘  └──────┬───────┘  └──────┬──────┘  └──────┬───────┘
       │                 │                 │                │
       │  1. Install/    │                 │                │
       │     Update SW   │                 │                │
       ├────────────────>│                 │                │
       │                 │                 │                │
       │                 │ 2. Start in     │                │
       │                 │    Verification │                │
       │                 │    Phase        │                │
       │                 │                 │                │
       │                 │ 3. Open Storage │                │
       │                 ├────────────────>│                │
       │                 │  (OpenKeyValue  │                │
       │                 │   Storage/      │                │
       │                 │   OpenFile      │                │
       │                 │   Storage)      │                │
       │                 │                 │                │
       │                 │                 │ 4. Check       │
       │                 │                 │    Version     │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │                 │ 5. Compare:    │
       │                 │                 │  - deployment  │
       │                 │                 │    version     │
       │                 │                 │  - contract    │
       │                 │                 │    version     │
       │                 │                 │                │
       │                 │ [如果需要更新]    │                │
       │                 │                 │ 6. Create      │
       │                 │                 │    Backup      │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │ 7. DataUpdate   │                │
       │                 │    Indication   │                │
       │                 │    Callback     │                │
       │                 │<────────────────┤                │
       │                 │                 │                │
       │                 │ 8. AppDataUpdate│                │
       │                 │    Callback     │                │
       │                 │<────────────────┤                │
       │                 │  (with Instance │                │
       │                 │   Specifier)    │                │
       │                 │                 │                │
       │                 │ 9. Manual Data  │                │
       │                 │    Migration    │                │
       │                 ├────────────────>│                │
       │                 │  - GetValue<old>│                │
       │                 │  - Convert      │                │
       │                 │  - SetValue<new>│                │
       │                 │                 │                │
       │                 │                 │ 10. Apply      │
       │                 │                 │     Updates    │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │ 11. OR Call     │                │
       │                 │     UpdatePer-  │                │
       │                 │     sistency()  │                │
       │                 ├────────────────>│                │
       │                 │                 │                │
       │                 │                 │ 12. Update All │
       │                 │                 │     Storages   │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │ 13. Verification│                 │                │
       │     Succeeded   │                 │                │
       │<────────────────┤                 │                │
       │                 │                 │                │
       │                 │ 14. CleanUp-    │                │
       │                 │     Persistency │                │
       │                 ├────────────────>│                │
       │                 │                 │                │
       │                 │                 │ 15. Remove     │
       │                 │                 │     Backup     │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │ 16. Finalize &  │                 │                │
       │     Restart in  │                 │                │
       │     Normal Mode │                 │                │
       ├────────────────>│                 │                │
       │                 │                 │                │
```

### 3.2 更新失败回滚流程

```
┌──────────────┐  ┌──────────────┐  ┌─────────────┐  ┌──────────────┐
│   UCM        │  │  Adaptive    │  │ Persistency │  │   Storage    │
│              │  │ Application  │  │             │  │   Backend    │
└──────┬───────┘  └──────┬───────┘  └──────┬──────┘  └──────┬───────┘
       │                 │                 │                │
       │  1. Update      │                 │                │
       │     Failed      │                 │                │
       ├────────────────>│                 │                │
       │                 │ (Verification   │                │
       │                 │  Failed)        │                │
       │                 │                 │                │
       │  2. Rollback    │                 │                │
       │     Initiated   │                 │                │
       ├────────────────>│                 │                │
       │                 │                 │                │
       │                 │ 3. Open Storage │                │
       │                 ├────────────────>│                │
       │                 │                 │                │
       │                 │                 │ 4. Detect      │
       │                 │                 │    Version     │
       │                 │                 │    Mismatch    │
       │                 │                 │  (manifest <   │
       │                 │                 │   stored)      │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │                 │ 5. Check       │
       │                 │                 │    Backup      │
       │                 │                 │    Version     │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │                 │ 6. Restore     │
       │                 │                 │    from Backup │
       │                 │                 ├───────────────>│
       │                 │                 │                │
       │                 │ 7. Storage      │                │
       │                 │    Restored     │                │
       │                 │<────────────────┤                │
       │                 │                 │                │
```

---

## 4. 关键 API 和触发机制

### 4.1 两种更新触发方式

#### 方式 1: 显式更新 (Explicit Update)
应用程序在验证阶段主动调用：

```cpp
// 应用程序在 Verification Phase 调用
ara::core::Result<void> result = ara::per::UpdatePersistency();
if (result.has_value()) {
    // 所有存储已更新成功
    ara::per::CleanUpPersistency();  // 清除备份
}
```

#### 方式 2: 隐式更新 (Implicit Update)
在打开存储时自动触发：

```cpp
// 应用程序正常打开存储
auto kvs = ara::per::OpenKeyValueStorage(instanceSpec);
// Persistency 自动检测版本并执行更新
```

### 4.2 核心 API 功能

| API 函数 | 触发时机 | 作用 |
|---------|---------|------|
| **RegisterDataUpdateIndication()** | 初始化时注册 | 注册回调函数，在每个存储元素更新时被调用 |
| **RegisterApplicationDataUpdateCallback()** | 初始化时注册 | 注册回调函数，在存储的契约版本变更时被调用，用于手动数据迁移 |
| **UpdatePersistency()** | 验证阶段 | 显式触发所有存储的更新 |
| **CleanUpPersistency()** | 更新成功后 | 清除所有备份数据 |
| **ResetPersistency()** | 需要时 | 将所有持久化数据恢复到初始状态（工厂重置） |
| **CheckForManifestUpdate()** | 轮询 | 检查清单是否有配置更新 |
| **ReloadPersistencyManifest()** | 检测到更新后 | 重新加载清单数据，标记存储需要更新 |

---

## 5. 版本管理机制

### 5.1 两种版本类型

#### Deployment Version (部署版本)
```cpp
PersistencyDeployment.version
```
- 由集成者设置
- 控制存储结构、冗余策略、加密配置的变更
- 变更触发自动更新

#### Contract Version (契约版本)
```cpp
PersistencyInterface.contractVersion
FunctionalClusterInteractsWithPersistencyDeploymentMapping.contractVersion
```
- 由应用开发者设置
- 表示数据格式和类型的接口版本
- 变更触发应用程序回调进行手动迁移

### 5.2 版本比较逻辑

```cpp
// SWS_PER_00386: Update Detection
if (manifest.deploymentVersion > stored.deploymentVersion) {
    // 需要更新
    CreateBackup();
    PerformUpdate();
    
    if (manifest.contractVersion > stored.contractVersion) {
        // 需要应用程序参与数据迁移
        TriggerAppDataUpdateCallback();
    }
}

// SWS_PER_00396: Rollback Detection
if (manifest.deploymentVersion < stored.deploymentVersion) {
    // 需要回滚
    if (manifest.deploymentVersion == backup.deploymentVersion) {
        RestoreBackup();
    } else {
        RemoveAllStorages();
        ReinstallFromManifest();
    }
}
```

---

## 6. 数据迁移示例

### 6.1 场景：mph to km/h 转换

**Version 1 (旧版本):**
```cpp
// 最高速度以 mph 为单位，存储为 uint8
uint8_t maxSpeed = 65;  // mph
```

**Version 2 (新版本):**
```cpp
// 最高速度以 km/h 为单位，存储为 uint16
uint16_t maxSpeed = 105;  // km/h
```

**迁移代码:**
```cpp
void MyApp::OnDataUpdate(const ara::core::InstanceSpecifier& storage) {
    auto kvs = ara::per::OpenKeyValueStorage(storage);
    
    // 1. 读取旧格式数据 (uint8, mph)
    auto oldValue = kvs->GetValue<uint8_t>("maxSpeed");
    if (oldValue.has_value()) {
        uint8_t speedMph = oldValue.value();
        
        // 2. 转换: mph → km/h
        uint16_t speedKmh = static_cast<uint16_t>(speedMph * 1.60934);
        
        // 3. 删除旧键
        kvs->RemoveKey("maxSpeed");
        
        // 4. 存储新格式数据 (uint16, km/h)
        kvs->SetValue<uint16_t>("maxSpeed", speedKmh);
        
        // 5. 同步到存储
        kvs->SyncToStorage();
    }
}
```

---

## 7. 更新策略

### 7.1 Key-Value Storage 更新策略

| 策略 | 行为 | 适用场景 |
|-----|------|---------|
| **overwrite** | 覆盖现有值 | 默认策略，配置数据更新 |
| **keepExisting** | 保留现有值 | 用户数据，不应被更新覆盖 |
| **delete** | 删除键值对 | 废弃的配置项 |

```cpp
// 在 Manifest 中配置
PersistencyKeyValueStorage.keyValuePair {
    key = "maxSpeed"
    initValue = "100"
    updateStrategy = overwrite  // or keepExisting, delete
}
```

### 7.2 File Storage 更新策略

| 策略 | 行为 | 适用场景 |
|-----|------|---------|
| **overwrite** | 覆盖现有文件 | 配置文件、模板文件 |
| **keepExisting** | 保留现有文件 | 用户创建的文件、日志文件 |
| **delete** | 删除文件 | 废弃的文件 |

---

## 8. 配置独立更新流程

### 8.1 场景
Update and Configuration Management 可以部署独立的配置更新，不涉及应用程序二进制文件。

### 8.2 流程

```cpp
// 应用程序定期检查配置更新
void MyApp::CheckConfigUpdate() {
    auto updateDetected = ara::per::CheckForManifestUpdate();
    
    if (updateDetected.has_value() && updateDetected.value()) {
        // 检测到清单更新
        
        // 1. 关闭所有打开的存储
        CloseAllStorages();
        
        // 2. 重新加载清单
        auto result = ara::per::ReloadPersistencyManifest();
        
        if (result.has_value()) {
            // 3. 选项 A: 触发完整更新
            ara::per::UpdatePersistency();
            
            // 或 选项 B: 重新打开各个存储（隐式更新）
            ReopenAllStorages();
        }
    }
}
```

### 8.3 检测机制

```cpp
// SWS_PER_00580
bool CheckForManifestUpdate() {
    // 比较存储的清单版本与初始化时读取的版本
    if (stored_manifest_version > initialized_manifest_version) {
        return true;  // 有更新
    }
    return false;
}
```

---

## 9. 冗余和加密配置的更新

### 9.1 冗余配置迁移

更新时，Persistency 需要处理冗余策略的变更：

```cpp
// SWS_PER_00560: 读取旧存储
// 使用存储元数据中保存的旧冗余配置读取数据
ReadWithOldRedundancy(storage.metadata.redundancy);

// SWS_PER_00561: 写入新存储
// 使用清单中的新冗余配置写入数据
WriteWithNewRedundancy(manifest.redundancy);
UpdateMetadata(manifest.redundancy);
```

### 9.2 加密配置迁移

```cpp
// SWS_PER_00559: 存储加密信息
// 在安装时保存加密密钥和算法信息到元数据
storage.metadata.encryptionKeys = manifest.encryptionKeys;
storage.metadata.encryptionAlgo = manifest.encryptionAlgo;

// 更新时：
// 1. 使用旧密钥解密
DecryptWithOldKeys(storage.metadata.encryptionKeys);

// 2. 使用新密钥加密
EncryptWithNewKeys(manifest.encryptionKeys);
```

---

## 10. 元数据管理

### 10.1 中心化存储位置

```cpp
// SWS_PER_00463
// Persistency 在中心位置存储所有存储的信息
Location: ProcessToMachineMapping.persistencyCentralStorageURI

Stored Information:
├── Key-Value Storages
│   ├── Storage ID
│   ├── Contract Version
│   ├── Deployment Version
│   ├── Redundancy Config
│   └── Encryption Config
├── File Storages
│   ├── Storage ID
│   ├── Contract Version
│   ├── Deployment Version
│   ├── Redundancy Config
│   └── Encryption Config
└── Manifest Version
    └── Current Deployment Version
```

### 10.2 元数据结构示例

```json
{
  "persistency_metadata": {
    "manifest_version": "2.0.0",
    "storages": [
      {
        "type": "KeyValueStorage",
        "instance_specifier": "/app/config/settings",
        "contract_version": "1.2.0",
        "deployment_version": "2.0.0",
        "redundancy": {
          "strategy": "CRC",
          "n_copies": 2
        },
        "encryption": {
          "algorithm": "AES256",
          "key_id": "app_key_001"
        },
        "last_updated": "2025-11-10T10:30:00Z"
      },
      {
        "type": "FileStorage",
        "instance_specifier": "/app/data/logs",
        "contract_version": "1.0.0",
        "deployment_version": "1.5.0",
        "redundancy": {
          "strategy": "None"
        },
        "last_updated": "2025-10-15T08:15:00Z"
      }
    ],
    "backup": {
      "exists": true,
      "version": "1.9.0",
      "created": "2025-11-10T10:25:00Z"
    }
  }
}
```

---

## 11. 实现建议

### 11.1 LightAP Persistency 需要实现的关键功能

#### Phase 2a: 版本管理 (高优先级)
```cpp
class CVersionManager {
public:
    // 版本比较
    static bool NeedsUpdate(
        const String& manifestVersion,
        const String& storedVersion
    );
    
    // 版本存储
    static Result<void> StoreVersion(
        const InstanceSpecifier& storage,
        const String& contractVersion,
        const String& deploymentVersion
    );
    
    // 版本读取
    static Result<VersionInfo> LoadVersion(
        const InstanceSpecifier& storage
    );
};
```

#### Phase 2b: 备份管理 (高优先级)
```cpp
class CBackupManager {
public:
    // 创建备份
    static Result<void> CreateBackup(
        const InstanceSpecifier& storage
    );
    
    // 恢复备份
    static Result<void> RestoreBackup(
        const InstanceSpecifier& storage
    );
    
    // 删除备份
    static Result<void> RemoveBackup(
        const InstanceSpecifier& storage
    );
    
    // 检查备份存在
    static Result<bool> BackupExists(
        const InstanceSpecifier& storage
    );
};
```

#### Phase 2c: 清单管理 (中优先级)
```cpp
class CManifestManager {
public:
    // 加载清单
    static Result<void> LoadManifest();
    
    // 重新加载清单
    static Result<void> ReloadManifest();
    
    // 检查清单更新
    static Result<bool> CheckManifestUpdate();
    
    // 获取存储配置
    static Result<StorageConfig> GetStorageConfig(
        const InstanceSpecifier& storage
    );
};
```

### 11.2 更新流程实现框架

```cpp
namespace lap {
namespace per {

Result<void> UpdatePersistency() {
    // 1. 获取所有已配置的存储
    auto storages = CManifestManager::GetAllStorages();
    
    for (const auto& storageSpec : storages) {
        // 2. 检查版本
        auto manifestVersion = CManifestManager::GetVersion(storageSpec);
        auto storedVersion = CVersionManager::LoadVersion(storageSpec);
        
        if (CVersionManager::NeedsUpdate(manifestVersion, storedVersion)) {
            // 3. 创建备份
            auto backupResult = CBackupManager::CreateBackup(storageSpec);
            if (!backupResult.has_value()) {
                return Result<void>::FromError(backupResult.error());
            }
            
            // 4. 执行更新
            auto updateResult = PerformStorageUpdate(storageSpec);
            if (!updateResult.has_value()) {
                // 更新失败，恢复备份
                CBackupManager::RestoreBackup(storageSpec);
                return Result<void>::FromError(updateResult.error());
            }
            
            // 5. 触发回调
            TriggerUpdateCallbacks(storageSpec);
            
            // 6. 更新版本信息
            CVersionManager::StoreVersion(
                storageSpec,
                manifestVersion.contractVersion,
                manifestVersion.deploymentVersion
            );
        }
    }
    
    return Result<void>();
}

Result<void> CleanUpPersistency() {
    auto storages = CManifestManager::GetAllStorages();
    
    for (const auto& storageSpec : storages) {
        // 删除所有备份
        CBackupManager::RemoveBackup(storageSpec);
    }
    
    return Result<void>();
}

Result<bool> CheckForManifestUpdate() {
    return CManifestManager::CheckManifestUpdate();
}

Result<void> ReloadPersistencyManifest() {
    return CManifestManager::ReloadManifest();
}

} // namespace per
} // namespace lap
```

---

## 12. 与其他功能集群的交互

### 12.1 依赖关系

```
┌─────────────────────────────────────────────────────┐
│         Update & Configuration Management           │
│              (UCM - ara::ucm)                        │
└──────────────┬──────────────────────────────────────┘
               │ 控制更新生命周期
               ▼
┌─────────────────────────────────────────────────────┐
│             Execution Management                     │
│              (EM - ara::exec)                        │
│  - 控制应用程序状态 (Verification/Normal)             │
│  - 启动/停止应用程序                                  │
└──────────────┬──────────────────────────────────────┘
               │ 进程控制
               ▼
┌─────────────────────────────────────────────────────┐
│              Persistency                             │
│              (ara::per)                              │
│  - 检测版本变更                                       │
│  - 执行数据迁移                                       │
│  - 管理备份/恢复                                      │
└──────────────┬──────────────────────────────────────┘
               │ 数据存储
               ▼
┌─────────────────────────────────────────────────────┐
│         Physical Storage (File System)              │
└─────────────────────────────────────────────────────┘
```

### 12.2 UCM 和 Persistency 的职责分工

| 组件 | 职责 |
|------|------|
| **UCM** | • 下载软件包<br>• 验证软件包完整性<br>• 协调更新流程<br>• 处理应用程序状态转换<br>• 决定回滚或继续 |
| **EM** | • 在 Verification Phase 启动应用<br>• 在 Normal Mode 启动应用<br>• 监控应用程序健康状态 |
| **Persistency** | • 检测数据版本<br>• 创建/恢复备份<br>• 执行数据迁移<br>• 触发应用回调<br>• 管理存储元数据 |
| **Application** | • 响应更新回调<br>• 执行业务逻辑相关的数据转换<br>• 验证更新结果<br>• 调用 UpdatePersistency/CleanUpPersistency |

---

## 13. 最佳实践

### 13.1 应用程序开发者

1. **注册回调函数**
   ```cpp
   // 在初始化时注册
   void MyApp::Initialize() {
       ara::per::RegisterApplicationDataUpdateCallback(
           [this](const ara::core::InstanceSpecifier& storage) {
               OnStorageUpdate(storage);
           }
       );
   }
   ```

2. **实现数据迁移逻辑**
   ```cpp
   void MyApp::OnStorageUpdate(const ara::core::InstanceSpecifier& storage) {
       // 根据 contract version 执行相应的迁移逻辑
       auto version = GetStoredContractVersion(storage);
       
       if (version == "1.0") {
           MigrateFrom_1_0_to_2_0(storage);
       } else if (version == "2.0") {
           MigrateFrom_2_0_to_3_0(storage);
       }
   }
   ```

3. **在验证阶段调用 UpdatePersistency**
   ```cpp
   void MyApp::VerificationPhase() {
       // 执行更新
       auto result = ara::per::UpdatePersistency();
       
       if (result.has_value()) {
           // 验证数据正确性
           if (ValidateData()) {
               // 清除备份
               ara::per::CleanUpPersistency();
               // 通知 UCM 验证成功
               NotifyUCM_Success();
           } else {
               // 通知 UCM 验证失败（触发回滚）
               NotifyUCM_Failure();
           }
       }
   }
   ```

### 13.2 集成者

1. **正确配置版本号**
   ```xml
   <PersistencyDeployment>
       <version>2.0.0</version>  <!-- Deployment Version -->
   </PersistencyDeployment>
   
   <PersistencyInterface>
       <contractVersion>1.2.0</contractVersion>  <!-- Contract Version -->
   </PersistencyInterface>
   ```

2. **选择合适的更新策略**
   - 配置数据: `overwrite`
   - 用户数据: `keepExisting`
   - 废弃数据: `delete`

3. **配置冗余和加密**
   ```xml
   <PersistencyDeployment>
       <redundancy>
           <strategy>CRC_N_FROM_M</strategy>
           <n>2</n>
           <m>3</m>
       </redundancy>
       <encryption>
           <algorithm>AES256</algorithm>
           <keyId>app_encryption_key</keyId>
       </encryption>
   </PersistencyDeployment>
   ```

---

## 14. 常见问题

### Q1: 更新失败会发生什么？
**A:** Persistency 会自动回滚到备份版本，确保数据一致性。如果备份版本与清单版本不匹配，则会删除所有数据并从清单重新安装。

### Q2: 如何处理大规模数据迁移？
**A:** 使用 `RegisterApplicationDataUpdateCallback` 让应用程序分批处理数据迁移，避免一次性加载所有数据到内存。

### Q3: 配置更新需要重启应用吗？
**A:** 不一定。可以使用 `CheckForManifestUpdate()` 轮询检测更新，然后调用 `ReloadPersistencyManifest()` 和重新打开存储，实现运行时更新。

### Q4: 如何保证更新的原子性？
**A:** Persistency 使用备份机制。只有当所有更新操作成功后，才会删除备份数据（通过 `CleanUpPersistency()`）。

### Q5: UpdatePersistency() 和 OpenStorage() 触发更新的区别？
**A:**
- `UpdatePersistency()`: 显式更新所有存储，适用于验证阶段集中处理
- `OpenStorage()`: 隐式更新单个存储，适用于按需更新

---

## 15. 总结

AUTOSAR Persistency 的 Update/Installation 机制是一个 **完整的 OTA 数据管理解决方案**，具有以下特点：

✅ **版本感知**: 自动检测部署版本和契约版本变更  
✅ **安全回滚**: 失败时自动恢复，保证数据一致性  
✅ **灵活迁移**: 支持应用程序自定义数据转换逻辑  
✅ **配置独立**: 支持运行时配置更新无需应用重启  
✅ **元数据管理**: 中心化存储版本和配置信息  
✅ **策略驱动**: 支持多种更新策略（覆盖/保留/删除）

对于 **LightAP Persistency** 来说，完整实现这套机制需要：
1. 版本管理器（读取/存储/比较版本）
2. 备份管理器（创建/恢复/删除备份）
3. 清单解析器（读取 Manifest 配置）
4. 更新协调器（协调整个更新流程）

这些是 Phase 2 的核心任务，建议优先实现版本管理和备份管理，然后逐步完善清单解析和更新协调逻辑。

---

**文档编制:** GitHub Copilot  
**技术审核:** 待审核  
**最后更新:** 2025-11-11
