# AUTOSAR KVS 存储目录结构分析与重构计划

## 文档信息
- **分析日期**: 2025-11-14
- **AUTOSAR 版本**: R24-11
- **文档来源**: AUTOSAR_AP_SWS_Persistency.pdf
- **当前实现状态**: 已完成 M-out-of-N 副本冗余重构

## 前置约束条件

### 1. 架构设计原则

**1.1 KVS Backend 分层设计**
- ✅ **保持 Backend 后端设计模式**
  - `PropertyBackend`: 处理 Property Tree 格式（内存数据库）
  - `SqliteBackend`: 处理 SQLite 数据库格式
  - `FileBackend`: 处理文件存储格式（JSON/二进制）
- ✅ **前端负责副本管理**
  - `CKeyValueStorage` (前端) 负责 Replica 操作
  - `CReplicaManager` 集成在 KVS 前端层
  - Backend 只关注单一数据源的读写
- ❌ **Backend 不直接处理副本**
  - Backend 接口保持纯粹的数据持久化职责
  - 跨 URI 副本分布由前端协调

**架构层次**:
```
┌─────────────────────────────────────────────────────────┐
│  Application Layer (ara::per API)                       │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  CKeyValueStorage (前端 - Frontend)                      │
│  • Replica 管理 (CReplicaManager)                       │
│  • 多 URI 协调和共识验证                                 │
│  • 元数据管理                                            │
│  • 事务管理                                              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────┬──────────────────┬───────────────────┐
│ PropertyBackend  │  SqliteBackend   │  FileBackend      │
│ (内存 Tree)      │  (SQLite DB)     │  (JSON/Binary)    │
│ • 单数据源读写   │  • SQL 操作      │  • 文件 I/O       │
│ • 数据序列化     │  • 事务支持      │  • JSON 解析      │
└──────────────────┴──────────────────┴───────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Core Layer (lap/core)                                   │
│  • File/Path (文件操作)                                  │
│  • Config (配置管理)                                     │
│  • Crypto (加密服务)                                     │
└─────────────────────────────────────────────────────────┘
```

### 2. Core 模块依赖

**2.1 优先使用 Core 类型和方法**
- ✅ **文件操作**: `Core::File::Util` 和 `Core::Path`
  - 替代: `std::filesystem`, `boost::filesystem`
  - 示例: `Core::File::Util::exists()`, `Core::Path::createDirectory()`
- ✅ **基础类型**: `Core::String`, `Core::Vector`, `Core::Map`
  - 替代: `std::string`, `std::vector`, `std::unordered_map`
- ✅ **内存管理**: `Core::UniqueHandle`, `Core::SharedHandle`
  - 替代: `std::unique_ptr`, `std::shared_ptr`
- ✅ **同步原语**: `Core::Mutex`, `Core::LockGuard`
  - 替代: `std::mutex`, `std::lock_guard`
- ✅ **结果类型**: `Core::Result<T>`
  - 统一的错误处理机制

**2.2 配置管理**
- ✅ **统一配置入口**: `Core::ConfigManager::getInstance()`
- ✅ **模块名统一**: `"persistency"`
- ✅ **配置获取**: `getModuleConfigJson("persistency")`
- ✅ **配置路径**: `config.json` 或环境变量指定
- ❌ **禁止**: 分散的配置文件、硬编码路径

**配置示例**:
```json
{
  "persistency": {
    "centralStorageURI": "/opt/autosar/persistency",
    "contractVersion": "3.0.0",
    "deploymentVersion": "1.0.0",
    "replicaCount": 3,
    "minValidReplicas": 2,
    "checksumType": "SHA256",
    "deploymentUris": [
      "/mnt/storage1/persistency",
      "/mnt/storage2/persistency",
      "/mnt/storage3/persistency"
    ],
    "encryptionEnabled": true,
    "encryptionAlgorithm": "AES-256-GCM",
    "storages": {
      "/app/kvs_instance": {
        "backendType": "file",
        "replicaCount": 3,
        "minValidReplicas": 2
      }
    }
  }
}
```

**2.3 加密服务**
- ✅ **统一加密接口**: `Core::Crypto`
- ✅ **算法获取**: 从 Core::Crypto 获取加密实现
- ✅ **密钥管理**: 使用 Core::Crypto 的密钥服务
- ❌ **禁止**: 自定义加密实现、OpenSSL 直接调用

**加密集成示例**:
```cpp
// 从 Core::Crypto 获取加密服务
auto& crypto = Core::Crypto::getInstance();

// 加密数据
Vector<UInt8> plainData = {...};
auto encryptResult = crypto.encrypt(
    "AES-256-GCM",           // 算法
    "persistency-key-id",    // 密钥 ID
    plainData
);

// 解密数据
auto decryptResult = crypto.decrypt(
    "AES-256-GCM",
    "persistency-key-id",
    encryptedData
);
```

### 3. 实现约束

**3.1 禁止事项**
- ❌ Backend 层不得直接调用 CReplicaManager
- ❌ 不得使用 `std::filesystem`（使用 `Core::Path`）
- ❌ 不得硬编码配置路径（使用 `Core::ConfigManager`）
- ❌ 不得自定义加密算法（使用 `Core::Crypto`）
- ❌ 不得绕过 Core 层直接调用系统 API

**3.2 必须事项**
- ✅ 所有文件操作必须通过 `Core::File::Util` 和 `Core::Path`
- ✅ 所有配置必须从 `Core::ConfigManager` 读取
- ✅ 所有加密操作必须通过 `Core::Crypto`
- ✅ Backend 接口保持纯粹（只负责单数据源持久化）
- ✅ 前端（CKeyValueStorage）负责副本协调和管理

## 1. AUTOSAR 规范要求

### 1.1 存储位置规范

根据 **[SWS_PER_00463]**:
```
Persistency shall store information about the installed Key-Value Storages
and File Storages in the location denoted by:
  ProcessToMachineMapping.persistencyCentralStorageURI

同时存储:
  - Contract Version
  - Deployment Version
  - Current Manifest
```

### 1.2 部署 URI 配置

根据 **[SWS_PER_00447]** 和相关规范:

**单一位置**:
- `PersistencyDeployment.deploymentUri` - 主存储位置

**多位置冗余** (M-out-of-N):
- 多个 `PersistencyDeployment.deploymentUris`（ordered）
- 配合 `PersistencyRedundancyMOutOfN` 使用
- 副本分布策略:
  - **Scope = Storage**: 整个存储的副本
    - `n=2`: 第一个位置存主副本，第二个位置存其他副本
    - `n>2`: 每个副本放在独立位置
  - **Scope = Element**: 每个键值对单独冗余

### 1.3 Key-Value Storage 标识

根据 **[SWS_PER_00383]**:
```cpp
// KVS 通过 shortName 路径标识
ara::core::InstanceSpecifier instanceSpec("/path/to/storage");
auto kvs = ara::per::OpenKeyValueStorage(instanceSpec, ...);
```

## 2. 当前实现分析

### 2.1 KeyValueStorage 当前目录结构

```
/tmp/test_kvs/                    # 测试用根目录
  └── kvs.json                    # 单一 JSON 文件（KvsFileBackend）
```

**问题**:
1. ❌ 没有实现 AUTOSAR 要求的层次化结构
2. ❌ 没有中央存储位置 (persistencyCentralStorageURI)
3. ❌ 缺少 manifest 版本信息存储
4. ❌ 缺少 contract/deployment version 管理
5. ❌ M-out-of-N 副本未按 URI 分布到不同位置

### 2.2 FileStorage 当前实现

#### 2.2.1 目录结构

```
{storageUri}/                     # 单一存储 URI
├── .metadata/                    # 元数据目录（已实现）
│   ├── storage_info.json         # 存储元数据
│   ├── replica_info.json         # 副本健康状态
│   └── file_registry.json        # 文件列表和校验和
├── current/                      # 当前活跃文件（已实现）
│   ├── file1.replica_0           # 副本 0
│   ├── file1.replica_1           # 副本 1
│   └── file1.replica_2           # 副本 2 (N=3, M=2)
├── backup/                       # 备份文件（已实现）
├── initial/                      # 初始文件（已实现）
├── update/                       # 更新临时文件（已实现）
└── shared/                       # 共享资源（可选）
```

**优势**:
- ✅ 已实现 M-out-of-N 副本冗余（CReplicaManager）
- ✅ 已实现元数据管理（FileStorageMetadata）
- ✅ 已实现版本管理（契约版本、部署版本、Manifest 版本）
- ✅ 已实现分类管理（current/backup/initial/update）
- ✅ 已集成 Core::Path 和 Core::File

**问题**:
1. ❌ 单一 URI 部署（未支持多 deploymentUris）
2. ❌ 副本在同一存储位置（未跨 URI 分布）
3. ❌ 未集成到中央存储 URI（persistencyCentralStorageURI）
4. ❌ 未注册到 Manifest 管理系统
5. ❌ 配置分散（部分在 ConfigManager，缺少统一配置入口）

#### 2.2.2 架构分析

**CFileStorageManager** (已实现特性):
- ✅ 目录结构管理（CreateDirectoryStructure）
- ✅ 版本比较和更新检测（NeedsUpdate, NeedsRollback）
- ✅ 备份管理（CreateBackup, RestoreBackup, RemoveBackup）
- ✅ 副本健康检查（CheckReplicaHealth）
- ✅ 副本修复（RepairReplicas）
- ✅ 元数据持久化（LoadMetadata, SaveMetadata）
- ✅ 更新事务管理（BeginUpdate, CommitUpdate, RollbackUpdate）

**CFileStorage** (已实现特性):
- ✅ AUTOSAR 标准 API 实现
- ✅ ConfigManager 集成（加载配置）
- ✅ 文件操作（OpenFileReadWrite, DeleteFile, RecoverFile, ResetFile）
- ✅ 文件信息查询（GetAllFileNames, FileExists, GetFileInfo）
- ✅ 资源管理（maxNumberOfFiles）

### 2.3 ReplicaManager 实现

```cpp
// 当前副本文件命名（已实现）
{storageUri}/{category}/file.replica_0
{storageUri}/{category}/file.replica_1
{storageUri}/{category}/file.replica_2
```

**优势**:
- ✅ M-out-of-N 冗余算法（共识投票）
- ✅ 校验和验证（CRC32/SHA256）
- ✅ 副本读取和写入
- ✅ 副本修复和同步

**问题**:
- ❌ 未实现多 URI 分布
- ❌ 副本元数据未存储到 Manifest
- ❌ 缺少副本分布策略配置

## 3. AUTOSAR 标准目录结构

### 3.1 推荐目录层次

```
{persistencyCentralStorageURI}/           # 中央存储根目录
├── manifest/                             # Manifest 元数据
│   ├── contract_version                  # 合约版本
│   ├── deployment_version                # 部署版本
│   └── installed_storages.json           # 已安装存储列表
│
├── kvs/                                  # Key-Value Storages 根目录
│   ├── {shortName_path_1}/               # KVS 实例 1
│   │   ├── metadata.json                 # 存储元数据
│   │   ├── data.json                     # 数据文件（或 data.db）
│   │   ├── backup/                       # 备份数据
│   │   │   └── data.json.backup
│   │   └── update/                       # 更新临时区
│   │       └── data.json.tmp
│   │
│   └── {shortName_path_2}/               # KVS 实例 2
│       └── ...
│
└── fs/                                   # File Storages 根目录
    └── {shortName_path}/                 # FS 实例
        ├── metadata.json
        ├── current/                      # 当前数据
        ├── backup/                       # 备份数据
        └── update/                       # 更新数据
```

### 3.2 多 URI 冗余结构 (M-out-of-N)

**配置示例** (N=3, M=2):
```json
{
  "deploymentUris": [
    "/mnt/storage1/persistency",  # URI 0 - 主位置
    "/mnt/storage2/persistency",  # URI 1 - 副本位置1
    "/mnt/storage3/persistency"   # URI 2 - 副本位置2
  ],
  "redundancyHandling": {
    "scope": "persistencyRedundancyHandlingScopeStorage",
    "redundancy": {
      "n": 3,
      "m": 2
    }
  }
}
```

**分布结构**:
```
/mnt/storage1/persistency/        # URI[0] - 主副本
├── manifest/
├── kvs/
│   └── app/kvs_instance/
│       ├── replica_0/            # 副本 0 (主)
│       │   └── data.json
│       └── metadata.json
│
/mnt/storage2/persistency/        # URI[1] - 副本 1
└── kvs/
    └── app/kvs_instance/
        └── replica_1/
            └── data.json
│
/mnt/storage3/persistency/        # URI[2] - 副本 2
└── kvs/
    └── app/kvs_instance/
        └── replica_2/
            └── data.json
```

### 3.3 元数据文件格式

**installed_storages.json**:
```json
{
  "version": "1.0",
  "contract_version": "3.0.0",
  "deployment_version": "1.0.0",
  "storages": [
    {
      "type": "kvs",
      "shortName": "/application/kvs_instance",
      "deploymentUris": [
        "/mnt/storage1/persistency/kvs/app/kvs_instance"
      ],
      "redundancy": {
        "type": "MOutOfN",
        "n": 3,
        "m": 2
      },
      "installed_at": "2025-11-14T12:00:00Z",
      "updated_at": "2025-11-14T14:30:00Z"
    }
  ]
}
```

**storage metadata.json**:
```json
{
  "shortName": "/application/kvs_instance",
  "contractVersion": "3.0.0",
  "deploymentVersion": "1.0.0",
  "manifestVersion": "1.0.0",
  "minimumSustainedSize": 2097152,
  "maximumAllowedSize": 209715200,
  "currentSize": 10240,
  "redundancy": {
    "type": "MOutOfN",
    "n": 3,
    "m": 2,
    "replicas": [
      {
        "id": 0,
        "uri": "/mnt/storage1/persistency/kvs/app/kvs_instance/replica_0",
        "valid": true,
        "checksum": "sha256:abc123...",
        "last_sync": "2025-11-14T15:00:00Z"
      },
      {
        "id": 1,
        "uri": "/mnt/storage2/persistency/kvs/app/kvs_instance/replica_1",
        "valid": true,
        "checksum": "sha256:abc123...",
        "last_sync": "2025-11-14T15:00:00Z"
      },
      {
        "id": 2,
        "uri": "/mnt/storage3/persistency/kvs/app/kvs_instance/replica_2",
        "valid": true,
        "checksum": "sha256:abc123...",
        "last_sync": "2025-11-14T15:00:00Z"
      }
    ]
  },
  "checksumType": "SHA256",
  "encryptionEnabled": false,
  "created_at": "2025-11-14T12:00:00Z",
  "updated_at": "2025-11-14T15:00:00Z"
}
```

## 4. 重构计划

### 4.1 Phase 1: 目录结构标准化 (2-3 天)

**目标**: 实现 AUTOSAR 标准目录层次

#### 4.1.1 新增 StoragePathManager 类
```cpp
class StoragePathManager {
public:
    // 获取中央存储根目录
    static String getCentralStorageURI();
    
    // 获取 manifest 目录
    static String getManifestPath();
    
    // 获取 KVS 根目录
    static String getKvsRootPath();
    
    // 获取特定 KVS 实例路径
    static String getKvsInstancePath(const InstanceSpecifier& spec);
    
    // 获取副本路径（支持多 URI）
    static Vector<String> getReplicaPaths(
        const InstanceSpecifier& spec,
        UInt32 replicaCount,
        const Vector<String>& deploymentUris
    );
    
    // 创建标准目录结构
    static Result<void> createStorageStructure(
        const InstanceSpecifier& spec
    );
};
```

#### 4.1.2 更新 CKvsFileBackend 构造函数
```cpp
KvsFileBackend::KvsFileBackend(core::StringView instancePath) {
    // 旧实现: 直接使用路径
    // m_strFile = instancePath + "/kvs.json";
    
    // 新实现: 使用标准路径
    String kvsPath = StoragePathManager::getKvsInstancePath(instancePath);
    m_strFile = kvsPath + "/data.json";
    m_metadataFile = kvsPath + "/metadata.json";
    m_backupPath = kvsPath + "/backup";
    m_updatePath = kvsPath + "/update";
    
    // 创建目录结构
    StoragePathManager::createStorageStructure(instancePath);
}
```

### 4.2 Phase 2: Manifest 管理 (2-3 天)

**目标**: 实现版本和存储清单管理

#### 4.2.1 新增 ManifestManager 类
```cpp
class ManifestManager {
public:
    // 初始化 manifest
    static Result<void> initialize();
    
    // 注册已安装的存储
    static Result<void> registerStorage(
        const String& shortName,
        const String& type,  // "kvs" or "fs"
        const StorageConfig& config
    );
    
    // 卸载存储
    static Result<void> unregisterStorage(const String& shortName);
    
    // 获取存储信息
    static Result<StorageInfo> getStorageInfo(const String& shortName);
    
    // 检查 manifest 更新
    static Result<Bool> checkForUpdate();
    
    // 获取/设置版本
    static String getContractVersion();
    static String getDeploymentVersion();
    static Result<void> updateVersions(
        const String& contractVersion,
        const String& deploymentVersion
    );
    
private:
    static String m_manifestPath;
    static nlohmann::json m_manifestData;
};
```

#### 4.2.2 更新 CPersistencyManager
```cpp
// CPersistencyManager::initialize()
Result<void> CPersistencyManager::initialize() {
    // 初始化 manifest
    auto manifestResult = ManifestManager::initialize();
    if (!manifestResult.HasValue()) {
        return manifestResult;
    }
    
    // ... 现有初始化逻辑
}

// 注册新存储时
Result<SharedHandle<KeyValueStorage>> CPersistencyManager::getKvsStorage(...) {
    // ... 创建存储
    
    // 注册到 manifest
    StorageConfig config{
        .deploymentUris = {strFolder.data()},
        .redundancy = {.type = "MOutOfN", .n = 3, .m = 2}
    };
    ManifestManager::registerStorage(strFolder.data(), "kvs", config);
    
    // ...
}
```

### 4.3 Phase 3: 多 URI 冗余支持 (3-4 天)

**目标**: 实现跨多个存储位置的副本分布（前端协调模式）

**重要**: 遵循约束条件 - Backend 保持纯粹，副本管理在前端

#### 4.3.1 更新 ReplicaConfig 结构
```cpp
struct ReplicaConfig {
    UInt32 n;                          // 总副本数
    UInt32 m;                          // 最小有效副本数
    ChecksumType checksumType;
    Vector<String> deploymentUris;     // 新增: 多个部署 URI
    
    // 获取副本 i 的存储 URI
    String getReplicaUri(UInt32 replicaId) const {
        if (deploymentUris.size() == 1) {
            return deploymentUris[0];
        } else if (deploymentUris.size() == 2) {
            // n=2: URI[0] 主副本, URI[1] 其他副本
            return replicaId == 0 ? deploymentUris[0] : deploymentUris[1];
        } else {
            // n>2: 每个副本独立位置
            return deploymentUris[replicaId % deploymentUris.size()];
        }
    }
};
```

#### 4.3.2 更新 CReplicaManager (前端副本协调器)
```cpp
// 生成副本路径（跨多 URI）
Vector<String> CReplicaManager::getReplicaPaths(const String& logicalName) {
    Vector<String> paths;
    for (UInt32 i = 0; i < m_config.n; ++i) {
        String baseUri = m_config.getReplicaUri(i);
        String replicaPath = baseUri + "/" + m_category + "/" + 
                             logicalName + ".replica_" + std::to_string(i);
        paths.push_back(replicaPath);
    }
    return paths;
}

// 写入时分布副本（使用 Core::File）
Result<void> CReplicaManager::Write(
    const String& logicalName,
    const UInt8* data,
    Size size
) {
    auto replicaPaths = getReplicaPaths(logicalName);
    
    for (UInt32 i = 0; i < m_config.n; ++i) {
        // 确保目录存在（使用 Core::Path）
        String dir = Core::Path::getFolder(replicaPaths[i]);
        auto createResult = Core::Path::createDirectory(dir);
        if (!createResult) {
            LAP_PER_LOG_ERROR << "Failed to create directory: " << dir;
            continue;
        }
        
        // 写入副本（使用 Core::File::Util）
        auto writeResult = Core::File::Util::writeAllBytes(
            replicaPaths[i], 
            data, 
            size
        );
        
        if (!writeResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to write replica " << i 
                             << ": " << replicaPaths[i];
        }
    }
    
    return Result<void>::FromValue();
}
```

#### 4.3.3 CKeyValueStorage 前端副本管理

**核心设计**: KVS 前端负责副本协调，Backend 只负责单数据源读写

```cpp
class CKeyValueStorage {
private:
    UniqueHandle<KvsBackend> m_backend;           // Backend 接口
    UniqueHandle<CReplicaManager> m_replicaMgr;   // 前端副本管理器
    String m_instancePath;
    ReplicaConfig m_replicaConfig;
    
public:
    // 初始化时创建副本管理器
    Result<Bool> initialize(
        StringView strConfig, 
        Bool bCreate,
        KvsBackendType type
    ) noexcept {
        // 1. 从 Core::ConfigManager 读取配置
        auto& config = Core::ConfigManager::getInstance();
        auto persistencyConfig = config.getModuleConfigJson("persistency");
        
        // 2. 解析副本配置
        m_replicaConfig.n = persistencyConfig.value("replicaCount", 3);
        m_replicaConfig.m = persistencyConfig.value("minValidReplicas", 2);
        
        if (persistencyConfig.contains("deploymentUris")) {
            for (const auto& uri : persistencyConfig["deploymentUris"]) {
                m_replicaConfig.deploymentUris.push_back(uri.get<String>());
            }
        }
        
        // 3. 创建副本管理器（前端）
        String kvsPath = StoragePathManager::getKvsInstancePath(m_instancePath);
        m_replicaMgr = Core::MakeUnique<CReplicaManager>(
            kvsPath + "/current",
            m_replicaConfig
        );
        
        // 4. 创建 Backend（不感知副本）
        switch (type) {
            case KvsBackendType::kvsFile:
                m_backend = Core::MakeUnique<CKvsFileBackend>(
                    kvsPath + "/current/data.json"  // 单一数据源路径
                );
                break;
            case KvsBackendType::kvsSqlite:
                m_backend = Core::MakeUnique<CKvsSqliteBackend>(
                    kvsPath + "/current/kvs.db"
                );
                break;
            case KvsBackendType::kvsProperty:
                m_backend = Core::MakeUnique<CKvsPropertyBackend>();
                break;
        }
        
        // 5. 从副本读取初始数据（前端负责共识）
        auto loadResult = loadFromReplicas();
        if (!loadResult.HasValue()) {
            LAP_PER_LOG_WARN << "Failed to load from replicas, creating new storage";
        }
        
        return Result<Bool>::FromValue(true);
    }
    
    // 同步到存储（前端协调副本写入）
    Result<void> SyncToStorage() const noexcept {
        // 1. Backend 序列化数据
        Vector<UInt8> serializedData;
        auto serializeResult = m_backend->serialize(serializedData);
        if (!serializeResult.HasValue()) {
            return serializeResult;
        }
        
        // 2. 前端写入所有副本
        auto replicaResult = m_replicaMgr->Write(
            "data.json",  // 或 "kvs.db"，根据 backend 类型
            serializedData.data(),
            serializedData.size()
        );
        
        if (!replicaResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to write replicas";
            return replicaResult;
        }
        
        // 3. 更新元数据
        updateMetadata();
        
        return Result<void>::FromValue();
    }
    
private:
    // 从副本加载数据（前端共识验证）
    Result<void> loadFromReplicas() {
        // 1. 读取所有副本
        Vector<Vector<UInt8>> replicaData;
        auto readResult = m_replicaMgr->ReadAll("data.json", replicaData);
        if (!readResult.HasValue()) {
            return readResult;
        }
        
        // 2. 共识验证（前端负责）
        Vector<UInt8> consensusData;
        auto consensusResult = m_replicaMgr->GetConsensusData(replicaData, consensusData);
        if (!consensusResult.HasValue()) {
            return consensusResult;
        }
        
        // 3. Backend 反序列化
        auto deserializeResult = m_backend->deserialize(consensusData);
        return deserializeResult;
    }
};
```

#### 4.3.4 Backend 接口保持纯粹

**CKvsFileBackend** (不感知副本):
```cpp
class CKvsFileBackend : public KvsBackend {
private:
    String m_filePath;  // 单一文件路径（由前端指定）
    ptree m_propertyTree;
    
public:
    CKvsFileBackend(StringView filePath) : m_filePath(filePath.data()) {}
    
    // Backend 只负责单文件读写
    Result<void> parseFromFile() override {
        // 使用 Core::File::Util 检查文件
        if (!Core::File::Util::exists(m_filePath)) {
            return Result<void>::FromError(PerErrc::kResourceNotAvailable);
        }
        
        // 读取单个文件（不关心副本）
        try {
            std::ifstream ifs(m_filePath.data());
            boost::property_tree::read_json(ifs, m_propertyTree);
        } catch (const std::exception& e) {
            return Result<void>::FromError(PerErrc::kParseError);
        }
        
        return Result<void>::FromValue();
    }
    
    Result<void> saveToFile() const override {
        // 确保目录存在（使用 Core::Path）
        String dir = m_filePath.substr(0, m_filePath.rfind('/'));
        Core::Path::createDirectory(dir);
        
        // 写入单个文件（不关心副本）
        try {
            std::ofstream ofs(m_filePath.data());
            boost::property_tree::write_json(ofs, m_propertyTree);
        } catch (const std::exception& e) {
            return Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        return Result<void>::FromValue();
    }
    
    // 新增: 序列化接口（供前端使用）
    Result<void> serialize(Vector<UInt8>& outData) const override {
        std::ostringstream oss;
        boost::property_tree::write_json(oss, m_propertyTree);
        String jsonStr = oss.str();
        
        outData.assign(jsonStr.begin(), jsonStr.end());
        return Result<void>::FromValue();
    }
    
    // 新增: 反序列化接口（供前端使用）
    Result<void> deserialize(const Vector<UInt8>& inData) override {
        String jsonStr(inData.begin(), inData.end());
        std::istringstream iss(jsonStr);
        
        try {
            boost::property_tree::read_json(iss, m_propertyTree);
        } catch (const std::exception& e) {
            return Result<void>::FromError(PerErrc::kParseError);
        }
        
        return Result<void>::FromValue();
    }
};
```

#### 4.3.5 更新 KvsBackend 接口

```cpp
class KvsBackend {
public:
    virtual ~KvsBackend() = default;
    
    // 原有接口（保持兼容）
    virtual Result<void> parseFromFile() = 0;
    virtual Result<void> saveToFile() const = 0;
    
    // 新增: 序列化/反序列化接口（供前端副本管理）
    virtual Result<void> serialize(Vector<UInt8>& outData) const = 0;
    virtual Result<void> deserialize(const Vector<UInt8>& inData) = 0;
    
    // KVS 数据操作接口（不变）
    virtual Result<String> GetValue(StringView key) = 0;
    virtual Result<void> SetValue(StringView key, StringView value) = 0;
    virtual Result<void> RemoveKey(StringView key) = 0;
    virtual Result<Vector<String>> GetAllKeys() const = 0;
    virtual Result<void> SyncToStorage() const = 0;
};
```

### 4.4 Phase 4: 元数据管理增强 (2 天)

**目标**: 实现完整的存储元数据管理

#### 4.4.1 新增 StorageMetadata 类
```cpp
class StorageMetadata {
public:
    String shortName;
    String contractVersion;
    String deploymentVersion;
    String manifestVersion;
    UInt64 minimumSustainedSize;
    UInt64 maximumAllowedSize;
    UInt64 currentSize;
    
    struct ReplicaInfo {
        UInt32 id;
        String uri;
        Bool valid;
        String checksum;
        String lastSync;
    };
    
    ReplicaConfig redundancy;
    Vector<ReplicaInfo> replicas;
    ChecksumType checksumType;
    Bool encryptionEnabled;
    String createdAt;
    String updatedAt;
    
    // 序列化/反序列化
    static Result<StorageMetadata> fromJson(const String& jsonPath);
    Result<void> toJson(const String& jsonPath) const;
    
    // 更新副本状态
    void updateReplicaStatus(UInt32 replicaId, Bool valid, const String& checksum);
};
```

#### 4.4.2 集成到 CKeyValueStorage
```cpp
class KeyValueStorage {
private:
    UniqueHandle<KvsBackend> m_pKvsBackend;
    UniqueHandle<StorageMetadata> m_metadata;  // 新增
    
public:
    // 初始化时加载元数据
    Result<Bool> initialize(...) {
        // 加载或创建元数据
        String metadataPath = StoragePathManager::getKvsInstancePath(m_strPath) 
                              + "/metadata.json";
        
        if (File::Util::exists(metadataPath)) {
            auto result = StorageMetadata::fromJson(metadataPath);
            if (result.HasValue()) {
                m_metadata = std::make_unique<StorageMetadata>(result.Value());
            }
        } else {
            // 创建新元数据
            m_metadata = std::make_unique<StorageMetadata>();
            m_metadata->shortName = m_strPath.data();
            // ... 初始化其他字段
        }
        
        // ... 现有初始化逻辑
    }
    
    // 同步时更新元数据
    Result<void> SyncToStorage() const {
        auto result = m_pKvsBackend->SyncToStorage();
        if (result.HasValue()) {
            // 更新元数据
            m_metadata->updatedAt = getCurrentTimestamp();
            m_metadata->currentSize = calculateCurrentSize();
            
            String metadataPath = StoragePathManager::getKvsInstancePath(m_strPath) 
                                  + "/metadata.json";
            m_metadata->toJson(metadataPath);
        }
        return result;
    }
};
```

### 4.5 Phase 5: 配置系统集成 (1-2 天)

**目标**: 从 ConfigManager 读取 deploymentUris 配置

#### 4.5.1 更新配置格式
```json
{
  "persistency": {
    "centralStorageURI": "/opt/autosar/persistency",
    "contractVersion": "3.0.0",
    "deploymentVersion": "1.0.0",
    "storages": {
      "/application/kvs_instance": {
        "deploymentUris": [
          "/mnt/storage1/persistency",
          "/mnt/storage2/persistency",
          "/mnt/storage3/persistency"
        ],
        "redundancy": {
          "type": "MOutOfN",
          "n": 3,
          "m": 2,
          "scope": "storage"
        },
        "replicaCount": 3,
        "minValidReplicas": 2,
        "checksumType": "SHA256",
        "minimumSustainedSize": 2097152,
        "maximumAllowedSize": 209715200
      }
    }
  }
}
```

#### 4.5.2 更新 CFileStorage 配置加载
```cpp
Result<void> CFileStorage::initialize() {
    auto& config = ConfigManager::getInstance();
    auto persistencyConfig = config.getModuleConfigJson("persistency");
    
    if (persistencyConfig.contains("centralStorageURI")) {
        String centralUri = persistencyConfig["centralStorageURI"];
        StoragePathManager::setCentralStorageURI(centralUri);
    }
    
    // 加载存储特定配置
    if (persistencyConfig.contains("storages")) {
        auto storages = persistencyConfig["storages"];
        if (storages.contains(m_strPath.data())) {
            auto storageConfig = storages[m_strPath.data()];
            
            // 加载 deploymentUris
            if (storageConfig.contains("deploymentUris")) {
                for (const auto& uri : storageConfig["deploymentUris"]) {
                    m_deploymentUris.push_back(uri.get<String>());
                }
            }
            
            // 加载冗余配置
            if (storageConfig.contains("redundancy")) {
                auto redundancy = storageConfig["redundancy"];
                m_replicaConfig.n = redundancy["n"];
                m_replicaConfig.m = redundancy["m"];
                m_replicaConfig.deploymentUris = m_deploymentUris;
            }
            
            // ... 其他配置
        }
    }
    
    return Result<void>::FromValue();
}
```

## 5. 测试计划

### 5.1 单元测试更新

#### 5.1.1 StoragePathManager 测试
```cpp
TEST(StoragePathManagerTest, GetCentralStorageURI) {
    auto uri = StoragePathManager::getCentralStorageURI();
    EXPECT_FALSE(uri.empty());
    EXPECT_TRUE(uri.find("/persistency") != String::npos);
}

TEST(StoragePathManagerTest, GetKvsInstancePath) {
    InstanceSpecifier spec("/app/kvs_instance");
    auto path = StoragePathManager::getKvsInstancePath(spec);
    EXPECT_TRUE(path.find("/kvs/app/kvs_instance") != String::npos);
}

TEST(StoragePathManagerTest, CreateStorageStructure) {
    InstanceSpecifier spec("/test/kvs");
    auto result = StoragePathManager::createStorageStructure(spec);
    ASSERT_TRUE(result.HasValue());
    
    // 验证目录创建
    String basePath = StoragePathManager::getKvsInstancePath(spec);
    EXPECT_TRUE(Path::isDirectory(basePath));
    EXPECT_TRUE(Path::isDirectory(basePath + "/backup"));
    EXPECT_TRUE(Path::isDirectory(basePath + "/update"));
}
```

#### 5.1.2 ManifestManager 测试
```cpp
TEST(ManifestManagerTest, Initialize) {
    auto result = ManifestManager::initialize();
    ASSERT_TRUE(result.HasValue());
    
    String manifestPath = StoragePathManager::getManifestPath();
    EXPECT_TRUE(File::Util::exists(manifestPath + "/installed_storages.json"));
}

TEST(ManifestManagerTest, RegisterStorage) {
    StorageConfig config{
        .deploymentUris = {"/mnt/storage1"},
        .redundancy = {.type = "MOutOfN", .n = 3, .m = 2}
    };
    
    auto result = ManifestManager::registerStorage("/test/kvs", "kvs", config);
    ASSERT_TRUE(result.HasValue());
    
    auto info = ManifestManager::getStorageInfo("/test/kvs");
    ASSERT_TRUE(info.HasValue());
    EXPECT_EQ(info.Value().shortName, "/test/kvs");
}
```

#### 5.1.3 多 URI 冗余测试
```cpp
TEST(ReplicaManagerTest, MultiURIDistribution) {
    Vector<String> uris = {
        "/mnt/storage1/persistency",
        "/mnt/storage2/persistency",
        "/mnt/storage3/persistency"
    };
    
    ReplicaConfig config{
        .n = 3,
        .m = 2,
        .deploymentUris = uris
    };
    
    auto replicaMgr = std::make_unique<CReplicaManager>("/test", config);
    
    String data = "Test data for multi-URI";
    auto result = replicaMgr->Write("test_file.txt",
        reinterpret_cast<const UInt8*>(data.c_str()),
        data.length());
    ASSERT_TRUE(result.HasValue());
    
    // 验证副本分布在不同 URI
    EXPECT_TRUE(File::Util::exists("/mnt/storage1/persistency/test/test_file.txt.replica_0"));
    EXPECT_TRUE(File::Util::exists("/mnt/storage2/persistency/test/test_file.txt.replica_1"));
    EXPECT_TRUE(File::Util::exists("/mnt/storage3/persistency/test/test_file.txt.replica_2"));
}
```

### 5.2 集成测试

#### 5.2.1 完整流程测试
```cpp
TEST(IntegrationTest, KVSWithStandardStructure) {
    // 1. 初始化 Persistency
    ManifestManager::initialize();
    
    // 2. 打开 KVS
    auto kvs = OpenKeyValueStorage(
        InstanceSpecifier("/app/test_kvs"),
        true,  // create
        KvsBackendType::kvsFile
    );
    ASSERT_TRUE(kvs.HasValue());
    
    // 3. 设置数据
    EXPECT_TRUE(kvs.Value()->SetValue("key1", String("value1")).HasValue());
    EXPECT_TRUE(kvs.Value()->SyncToStorage().HasValue());
    
    // 4. 验证目录结构
    String basePath = StoragePathManager::getCentralStorageURI();
    EXPECT_TRUE(Path::isDirectory(basePath + "/kvs/app/test_kvs"));
    EXPECT_TRUE(File::Util::exists(basePath + "/kvs/app/test_kvs/data.json"));
    EXPECT_TRUE(File::Util::exists(basePath + "/kvs/app/test_kvs/metadata.json"));
    
    // 5. 验证 manifest 注册
    auto info = ManifestManager::getStorageInfo("/app/test_kvs");
    ASSERT_TRUE(info.HasValue());
}
```

## 6. 实施策略

### 6.1 重构方式说明

**⚠️ 重要**: 本次重构不考虑向后兼容，直接使用重构后的版本作为最新版本。

**实施原则**:
- ✅ 直接实施新的目录结构和架构
- ✅ 所有测试使用新的路径和接口
- ✅ 移除旧的实现代码
- ❌ 不保留旧路径支持
- ❌ 不提供数据迁移工具

**原因**:
1. 当前处于开发阶段，无历史生产数据需要迁移
2. 简化实施复杂度，加快重构速度
3. 避免维护双套代码带来的技术债务
4. 确保所有功能都基于新架构实现

### 6.2 测试数据处理

**清理策略**:
```bash
# 清理旧测试数据
rm -rf /tmp/test_kvs
rm -rf /tmp/test_file_storage

# 使用新路径进行测试
# 新路径将由 StoragePathManager 统一管理
```

**测试更新**:
- 所有测试用例直接使用新的 API 和路径
- 移除依赖旧路径的测试代码
- 更新测试配置使用 Core::ConfigManager

## 7. 工作量评估

| Phase | 任务 | 工作量 | 优先级 | 影响模块 |
|-------|------|--------|--------|----------|
| Phase 1 | 目录结构标准化 | 2-3 天 | P0 | KVS + FS |
| Phase 2 | Manifest 管理 | 2-3 天 | P0 | 通用 |
| Phase 3 | 多 URI 冗余（统一架构） | 4-5 天 | P1 | KVS + FS + Replica |
| Phase 4 | 元数据管理增强 | 2 天 | P1 | KVS (FS已实现) |
| Phase 5 | 配置集成 | 1-2 天 | P2 | 通用 |
| Phase 6 | FileStorage 集成中央存储 | 2 天 | P1 | FS |
| 测试 | 单元+集成测试 | 3-4 天 | P0 | 全部 |
| 文档 | 更新文档 | 1-2 天 | P2 | 全部 |
| **总计** | | **17-23 天** | |

### 7.1 详细任务分解

#### Phase 1: 目录结构标准化 (2-3 天)
- **KVS**: 从单文件 → 标准目录层次
  - 创建 StoragePathManager
  - 更新 CKvsFileBackend 路径管理
- **FileStorage**: 集成到中央存储 URI
  - 更新 CFileStorageManager 路径构建
  - 迁移现有测试数据
- **测试影响**: 57 个 KVS 测试需更新路径

#### Phase 2: Manifest 管理 (2-3 天)
- 创建 ManifestManager 类（150-200 行）
- 实现存储注册/卸载
- 集成到 CPersistencyManager
- **测试影响**: 新增 20+ Manifest 测试

#### Phase 3: 多 URI 冗余（统一架构）(4-5 天)
- **ReplicaManager 增强**:
  - 添加 deploymentUris 支持
  - 实现跨 URI 副本分布
  - 更新读写逻辑（200-300 行）
- **CKvsFileBackend 重构**:
  - 集成 CReplicaManager（替换单文件 JSON）
  - 统一 KVS 和 FileStorage 架构（150-200 行）
- **CFileStorageManager 更新**:
  - 支持多 URI 配置传递
  - 更新 Initialize 方法签名
- **测试影响**: 
  - 11 个 ReplicaManager 测试需扩展
  - 57 个 KVS 测试需验证副本行为
  - 新增多 URI 集成测试（10+）

#### Phase 4: 元数据管理增强 (2 天)
- 为 KVS 添加 StorageMetadata 类（FileStorage 已实现）
- 集成到 CKeyValueStorage
- 更新 SyncToStorage 保存元数据
- **测试影响**: KVS 元数据测试（5-10 个）

#### Phase 5: 配置集成 (1-2 天)
- 统一配置格式（JSON schema）
- 从 ConfigManager 读取 deploymentUris
- 更新 CFileStorage 和 CKeyValueStorage 初始化
- **测试影响**: 配置加载测试（5+）

#### Phase 6: FileStorage 集成中央存储 (2 天)
- 更新 OpenFileStorage 使用 StoragePathManager
- 注册到 ManifestManager
- 验证与 KVS 路径一致性
- **测试影响**: 更新现有 FileStorage 测试路径

### 7.2 测试更新工作量

| 测试套件 | 现有测试数 | 需更新 | 新增 | 预计工时 |
|---------|----------|--------|------|---------|
| KeyValueStorageTest | 57 | 40 | 10 | 1.5 天 |
| ReplicaManagerTest | 11 | 11 | 15 | 1 天 |
| FileStorageTest | ~20 | 15 | 5 | 0.5 天 |
| ManifestManagerTest | 0 | 0 | 20 | 1 天 |
| IntegrationTest | ~5 | 5 | 10 | 1 天 |
| **总计** | ~93 | ~71 | ~60 | **5 天** |

## 8. 风险与缓解

### 8.1 风险清单

**1. 架构变更风险**
- **KVS**: 从直接文件 I/O → 通过 CReplicaManager
- **FileStorage**: 从独立存储 → 中央存储集成
- **影响**: 需要全面测试新架构

**2. 测试覆盖风险**
- 165 个现有测试需要大量更新
- 新增 60+ 测试需要编写
- **影响**: 测试遗漏，回归缺陷

**3. 性能影响风险**
- 多 URI 访问增加网络/磁盘 I/O
- 副本共识验证增加 CPU 开销
- **影响**: 读写延迟可能增加 2-3 倍

**4. 配置复杂度风险**
- deploymentUris 配置错误
- Manifest 版本不一致
- **影响**: 初始化失败，运行时错误

**5. Core 模块依赖风险**
- Core::Path, Core::File, Core::ConfigManager 必须可用
- API 变更可能影响实现
- **影响**: 编译失败，功能异常

### 8.2 缓解措施

#### 8.2.1 实施策略

**直接切换方式**:
- ✅ 清理所有旧测试数据
- ✅ 直接使用新的目录结构
- ✅ 一次性更新所有测试用例
- ✅ 移除旧的实现代码

**实施步骤**:
1. **准备阶段**: 代码审查和设计评审（1 周）
2. **实施阶段**: 按 Phase 1-6 顺序实施（3-4 周）
3. **测试阶段**: 完整测试套件验证（1 周）
4. **部署阶段**: 更新文档和部署（1 周）

#### 8.2.2 性能优化措施

**1. 异步副本写入**
```cpp
Result<void> CReplicaManager::WriteAsync(
    const String& logicalName,
    const UInt8* data,
    Size size
) {
    // 主副本同步写入
    auto primaryResult = writeReplica(
        getPrimaryReplicaPath(logicalName), 
        data, 
        size, 
        0
    );
    if (!primaryResult.HasValue()) {
        return primaryResult;
    }
    
    // 其他副本异步写入
    for (UInt32 i = 1; i < m_config.n; ++i) {
        std::async(std::launch::async, 
            [this, i, logicalName, data, size]() {
                auto path = getReplicaPath(logicalName, i);
                writeReplica(path, data, size, i);
            }
        );
    }
    
    return Result<void>::FromValue();
}
```

**2. 智能读取策略**
```cpp
Result<Vector<UInt8>> CReplicaManager::ReadOptimized(
    const String& logicalName
) {
    // 1. 尝试从本地 URI 读取
    String localUri = getLocalUri();
    auto localResult = readReplicaFromUri(logicalName, localUri);
    if (localResult.HasValue()) {
        return localResult;
    }
    
    // 2. 并行读取前 M 个副本
    Vector<std::future<Result<Vector<UInt8>>>> futures;
    for (UInt32 i = 0; i < m_config.m; ++i) {
        futures.push_back(
            std::async(std::launch::async, 
                [this, &logicalName, i]() {
                    return readReplicaFromUri(
                        logicalName, 
                        m_config.deploymentUris[i]
                    );
                }
            )
        );
    }
    
    // 3. 返回第一个有效结果
    for (auto& future : futures) {
        auto result = future.get();
        if (result.HasValue()) {
            return result;
        }
    }
    
    return Result<Vector<UInt8>>::FromError(
        PerErrc::kStorageCorrupted
    );
}
```

#### 8.2.3 测试策略

**测试原则**:
- 所有测试直接使用新架构
- 不保留旧实现的测试代码
- 按 Phase 增量更新测试

**测试流程**:
```bash
#!/bin/bash
# tools/run_phase_tests.sh

PHASE=$1

case $PHASE in
  1)
    echo "=== Phase 1: Directory Structure ==="
    ./test_storage_path_manager
    ./test_kvs_file_backend_paths
    ;;
  2)
    echo "=== Phase 2: Manifest Management ==="
    ./test_manifest_manager
    ./test_storage_registration
    ;;
  3)
    echo "=== Phase 3: Multi-URI Redundancy ==="
    ./test_replica_manager_multi_uri
    ./test_kvs_multi_uri
    ;;
  all)
    echo "=== Full Test Suite ==="
    ./test_all_persistency
    ;;
esac
```

**性能验证**:
- 建立性能基准（写入 1000 个键值对）
- 新实现延迟不超过基准的 3 倍
- 监控内存使用和 CPU 占用

#### 8.2.4 配置验证工具

```cpp
// tools/validate_persistency_config.cpp
class ConfigValidator {
public:
    Result<void> validate(const String& configPath) {
        auto config = loadConfig(configPath);
        
        // 1. 验证 deploymentUris
        for (const auto& uri : config.deploymentUris) {
            if (!Path::isAbsolute(uri)) {
                return Error("deploymentUri must be absolute path: " + uri);
            }
            if (!Path::exists(uri) && !canCreatePath(uri)) {
                return Error("deploymentUri not accessible: " + uri);
            }
        }
        
        // 2. 验证冗余配置
        if (config.n < config.m) {
            return Error("n must be >= m (n=" + std::to_string(config.n) + 
                        ", m=" + std::to_string(config.m) + ")");
        }
        if (config.deploymentUris.size() < config.n) {
            return Error("Not enough deploymentUris for replica count (need " + 
                        std::to_string(config.n) + ", got " + 
                        std::to_string(config.deploymentUris.size()) + ")");
        }
        
        // 3. 验证版本格式
        if (!isValidVersion(config.contractVersion)) {
            return Error("Invalid contractVersion format: " + config.contractVersion);
        }
        
        // 4. 验证存储容量
        for (const auto& uri : config.deploymentUris) {
            auto freeSpace = Path::getFreeSpace(uri);
            if (freeSpace < config.minimumSustainedSize) {
                return Error("Insufficient space on " + uri + ": " + 
                            std::to_string(freeSpace) + " < " + 
                            std::to_string(config.minimumSustainedSize));
            }
        }
        
        return Result<void>::FromValue();
    }
};
```

## 9. 下一步行动

### 立即行动 (本周)
1. ✅ 完成文档扫描和分析
2. 📋 评审重构计划
3. 🏗️ 创建 StoragePathManager 骨架
4. 🧪 编写第一组单元测试

### 短期目标 (2 周内)
1. 完成 Phase 1 & 2（目录结构 + Manifest）
2. 更新 50% 现有测试
3. 提交第一个可测试版本

### 中期目标 (1 个月内)
1. 完成所有 5 个 Phase
2. 通过所有 165+ 测试
3. 完成性能基准测试
4. 更新用户文档

## 10. 参考资料

- AUTOSAR_AP_SWS_Persistency.pdf (R24-11)
- Current Implementation: `/modules/Persistency/`
- Test Suite: `/modules/Persistency/test/unittest/`
- ConfigManager: `/modules/Core/source/src/CConfig.cpp`
