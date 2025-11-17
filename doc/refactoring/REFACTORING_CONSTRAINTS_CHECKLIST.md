# Persistency 重构约束条件检查清单

## 文档版本
- **创建日期**: 2025-11-14
- **用途**: 重构实施过程中的强制性约束检查
- **状态**: 强制执行

---

## 1. 架构设计约束 ✅

### 1.1 KVS Backend 分层设计（强制）

#### ✅ Backend 职责边界
- [ ] **PropertyBackend** 只负责内存 Property Tree 操作
- [ ] **SqliteBackend** 只负责 SQLite 数据库操作
- [ ] **FileBackend** 只负责单文件读写（JSON/二进制）
- [ ] Backend 不得引用 `CReplicaManager`
- [ ] Backend 不得处理多副本逻辑
- [ ] Backend 不得实现跨 URI 操作

#### ✅ 前端职责边界
- [ ] `CKeyValueStorage` 集成 `CReplicaManager`
- [ ] 前端负责副本共识验证
- [ ] 前端负责多 URI 协调
- [ ] 前端负责元数据管理
- [ ] 前端负责事务管理

#### ✅ 接口设计
```cpp
// Backend 接口必须包含
class KvsBackend {
    virtual Result<void> serialize(Vector<UInt8>& outData) const = 0;
    virtual Result<void> deserialize(const Vector<UInt8>& inData) = 0;
    // ... 其他数据操作接口
};
```

**检查点**:
- [ ] Backend 接口中没有副本相关参数
- [ ] Backend 实现中没有调用 `CReplicaManager`
- [ ] 前端代码包含副本管理逻辑
- [ ] serialize/deserialize 接口已实现

---

## 2. Core 模块依赖约束 ✅

### 2.1 文件操作（强制）

#### ✅ 必须使用
```cpp
// 文件存在检查
Core::File::Util::exists(path)

// 文件读取
Core::File::Util::readAllBytes(path, data)

// 文件写入
Core::File::Util::writeAllBytes(path, data, size)

// 目录创建
Core::Path::createDirectory(path)

// 路径拼接
Core::Path::append(base, relative)

// 目录检查
Core::Path::isDirectory(path)

// 获取文件名
Core::Path::getFileName(path)

// 获取文件夹路径
Core::Path::getFolder(path)
```

#### ❌ 禁止使用
```cpp
// 禁止
std::filesystem::exists()
std::filesystem::create_directories()
boost::filesystem::path()
std::ofstream ofs(path);
std::ifstream ifs(path);
fopen(), fread(), fwrite()
```

**检查点**:
- [ ] 所有文件操作使用 `Core::File::Util`
- [ ] 所有路径操作使用 `Core::Path`
- [ ] 代码中无 `#include <filesystem>`
- [ ] 代码中无 `#include <boost/filesystem.hpp>`
- [ ] 代码中无直接使用 `std::ofstream`/`std::ifstream`

### 2.2 配置管理（强制）

#### ✅ 必须使用
```cpp
// 获取 ConfigManager 实例
auto& config = Core::ConfigManager::getInstance();

// 读取 persistency 模块配置
auto persistencyConfig = config.getModuleConfigJson("persistency");

// 访问配置项
String centralUri = persistencyConfig["centralStorageURI"].get<String>();
UInt32 replicaCount = persistencyConfig.value("replicaCount", 3);

if (persistencyConfig.contains("deploymentUris")) {
    for (const auto& uri : persistencyConfig["deploymentUris"]) {
        m_deploymentUris.push_back(uri.get<String>());
    }
}
```

#### ❌ 禁止使用
```cpp
// 禁止
std::ifstream configFile("config.json");
const char* CONFIG_PATH = "/etc/persistency/config.json";
parseConfigFile("/path/to/config.json");
```

**配置结构规范**:
```json
{
  "persistency": {
    "centralStorageURI": "/opt/autosar/persistency",
    "deploymentUris": [
      "/mnt/storage1/persistency",
      "/mnt/storage2/persistency",
      "/mnt/storage3/persistency"
    ],
    "replicaCount": 3,
    "minValidReplicas": 2,
    "checksumType": "SHA256",
    "encryptionEnabled": true,
    "encryptionAlgorithm": "AES-256-GCM",
    "encryptionKeyId": "persistency-master-key",
    "storages": {
      "/app/kvs_instance": {
        "backendType": "file",
        "replicaCount": 3
      }
    }
  }
}
```

**检查点**:
- [ ] 所有配置通过 `Core::ConfigManager` 读取
- [ ] 模块名统一使用 `"persistency"`
- [ ] 无硬编码配置路径
- [ ] 无分散的配置文件
- [ ] 配置格式符合 JSON schema

### 2.3 加密服务（强制）

#### ✅ 必须使用
```cpp
// 获取 Crypto 实例
auto& crypto = Core::Crypto::getInstance();

// 加密数据
auto encryptResult = crypto.encrypt(
    "AES-256-GCM",           // 从配置读取算法
    "persistency-key-id",    // 从配置读取密钥 ID
    plainData
);

// 解密数据
auto decryptResult = crypto.decrypt(
    "AES-256-GCM",
    "persistency-key-id",
    encryptedData
);

// 计算校验和
auto checksumResult = crypto.computeChecksum(
    "SHA256",                // 从配置读取算法
    data
);
```

#### ❌ 禁止使用
```cpp
// 禁止
#include <openssl/aes.h>
#include <openssl/sha.h>
EVP_EncryptInit_ex()
SHA256_Init()
自定义加密算法实现
```

**检查点**:
- [ ] 所有加密操作通过 `Core::Crypto`
- [ ] 加密算法从配置读取
- [ ] 密钥 ID 从配置读取
- [ ] 代码中无 OpenSSL 直接调用
- [ ] 代码中无自定义加密实现

### 2.4 基础类型（强制）

#### ✅ 必须使用
```cpp
// 字符串
Core::String str = "example";

// 向量
Core::Vector<UInt8> data;
Core::Vector<String> keys;

// 映射
Core::Map<String, String> kvMap;

// 智能指针
Core::UniqueHandle<KvsBackend> backend;
Core::SharedHandle<FileStorage> storage;

// 同步
Core::Mutex mutex;
Core::LockGuard lock(mutex);

// 结果类型
Core::Result<String> result;
```

#### ❌ 禁止使用
```cpp
// 禁止
std::string str;
std::vector<uint8_t> data;
std::unordered_map<std::string, std::string> map;
std::unique_ptr<KvsBackend> backend;
std::shared_ptr<FileStorage> storage;
std::mutex mutex;
std::lock_guard<std::mutex> lock;
```

**检查点**:
- [ ] 使用 `Core::String` 替代 `std::string`
- [ ] 使用 `Core::Vector` 替代 `std::vector`
- [ ] 使用 `Core::Map` 替代 `std::unordered_map`
- [ ] 使用 `Core::UniqueHandle` 替代 `std::unique_ptr`
- [ ] 使用 `Core::SharedHandle` 替代 `std::shared_ptr`
- [ ] 使用 `Core::Mutex` 替代 `std::mutex`
- [ ] 使用 `Core::Result` 作为返回类型

---

## 3. 实现约束检查 ✅

### 3.1 Backend 实现检查清单

**CKvsFileBackend.cpp**:
- [ ] 没有引用 `CReplicaManager.hpp`
- [ ] 没有 `deploymentUris` 相关代码
- [ ] 只有单文件路径成员变量
- [ ] 使用 `Core::File::Util` 进行文件操作
- [ ] 使用 `Core::Path` 进行路径操作
- [ ] 实现了 `serialize()` 接口
- [ ] 实现了 `deserialize()` 接口
- [ ] `parseFromFile()` 只读取单个文件
- [ ] `saveToFile()` 只写入单个文件

**CKvsSqliteBackend.cpp**:
- [ ] 没有引用 `CReplicaManager.hpp`
- [ ] 只有单数据库路径
- [ ] 使用 `Core::Path` 创建数据库目录
- [ ] 实现了 `serialize()` 接口
- [ ] 实现了 `deserialize()` 接口

**CKvsPropertyBackend.cpp**:
- [ ] 纯内存操作，无文件依赖
- [ ] 实现了 `serialize()` 接口
- [ ] 实现了 `deserialize()` 接口

### 3.2 前端实现检查清单

**CKeyValueStorage.cpp**:
- [ ] 包含 `CReplicaManager.hpp`
- [ ] 有 `m_replicaMgr` 成员变量
- [ ] 从 `Core::ConfigManager` 读取配置
- [ ] 模块名使用 `"persistency"`
- [ ] 初始化时创建 `CReplicaManager`
- [ ] `SyncToStorage()` 通过副本管理器写入
- [ ] 有副本共识验证逻辑
- [ ] 有多 URI 协调逻辑
- [ ] Backend 通过前端指定的单一路径创建

**CFileStorage.cpp**:
- [ ] 从 `Core::ConfigManager` 读取配置
- [ ] 模块名使用 `"persistency"`
- [ ] 使用 `CFileStorageManager` 管理副本
- [ ] 配置传递给 `CFileStorageManager`

### 3.3 ReplicaManager 实现检查清单

**CReplicaManager.cpp**:
- [ ] 使用 `Core::File::Util` 进行所有文件操作
- [ ] 使用 `Core::Path` 进行所有路径操作
- [ ] 支持 `deploymentUris` 配置
- [ ] 实现跨 URI 副本分布
- [ ] 实现共识验证算法
- [ ] 如需加密，使用 `Core::Crypto`

### 3.4 配置加载检查清单

**所有配置读取代码**:
```cpp
// 标准配置读取模板
auto& config = Core::ConfigManager::getInstance();
auto persistencyConfig = config.getModuleConfigJson("persistency");

// 必须有默认值
UInt32 replicaCount = persistencyConfig.value("replicaCount", 3);

// 必须检查存在性
if (persistencyConfig.contains("deploymentUris")) {
    // ... 处理
}

// 必须有错误处理
try {
    // ... 配置读取
} catch (const std::exception& e) {
    LAP_PER_LOG_ERROR << "Failed to load config: " << e.what();
    // 使用默认配置
}
```

**检查点**:
- [ ] 所有配置读取使用 `Core::ConfigManager`
- [ ] 模块名统一为 `"persistency"`
- [ ] 所有配置项有默认值
- [ ] 所有配置访问有异常处理
- [ ] 配置错误时使用默认值而非失败

---

## 4. 测试约束检查 ✅

### 4.1 单元测试要求

**Backend 测试**:
- [ ] 测试单数据源读写
- [ ] 测试 `serialize()`/`deserialize()`
- [ ] 测试文件不存在情况
- [ ] 测试数据损坏情况
- [ ] 不测试副本相关功能

**前端测试**:
- [ ] 测试副本创建
- [ ] 测试副本共识
- [ ] 测试多 URI 分布
- [ ] 测试配置加载
- [ ] 测试加密集成（如启用）

**ReplicaManager 测试**:
- [ ] 测试单 URI 副本
- [ ] 测试多 URI 副本
- [ ] 测试副本损坏恢复
- [ ] 测试共识算法
- [ ] 测试使用 `Core::File::Util`

### 4.2 集成测试要求

**端到端测试**:
- [ ] 测试完整的 KVS 创建和读写流程
- [ ] 测试配置从 `Core::ConfigManager` 加载
- [ ] 测试多 URI 副本分布
- [ ] 测试副本故障恢复
- [ ] 测试加密存储（如启用）

---

## 5. 代码审查检查清单 ✅

### 5.1 禁止项检查

**搜索以下内容，如发现则违反约束**:
```bash
# 禁止的文件系统库
grep -r "std::filesystem" modules/Persistency/source/
grep -r "boost::filesystem" modules/Persistency/source/
grep -r "#include <filesystem>" modules/Persistency/source/

# 禁止的文件操作
grep -r "std::ofstream" modules/Persistency/source/
grep -r "std::ifstream" modules/Persistency/source/
grep -r "fopen\|fread\|fwrite" modules/Persistency/source/

# 禁止的加密库
grep -r "#include <openssl" modules/Persistency/source/
grep -r "EVP_" modules/Persistency/source/
grep -r "SHA256_" modules/Persistency/source/

# 禁止的标准库类型
grep -r "std::string " modules/Persistency/source/ | grep -v "// OK:"
grep -r "std::vector<" modules/Persistency/source/ | grep -v "// OK:"
grep -r "std::unique_ptr" modules/Persistency/source/
grep -r "std::shared_ptr" modules/Persistency/source/

# Backend 不应包含副本逻辑
grep -r "CReplicaManager" modules/Persistency/source/src/CKvsFileBackend.cpp
grep -r "deploymentUris" modules/Persistency/source/src/CKvsFileBackend.cpp
```

### 5.2 必需项检查

**验证以下内容存在**:
```bash
# 使用 Core 类型
grep -r "Core::File::Util" modules/Persistency/source/
grep -r "Core::Path::" modules/Persistency/source/
grep -r "Core::ConfigManager" modules/Persistency/source/
grep -r "Core::String" modules/Persistency/source/
grep -r "Core::Vector" modules/Persistency/source/

# 配置管理
grep -r 'getModuleConfigJson("persistency")' modules/Persistency/source/

# Backend 接口
grep -r "virtual.*serialize.*Vector<UInt8>" modules/Persistency/source/inc/
grep -r "virtual.*deserialize.*Vector<UInt8>" modules/Persistency/source/inc/

# 前端副本管理
grep -r "CReplicaManager" modules/Persistency/source/src/CKeyValueStorage.cpp
```

---

## 6. 实施流程检查 ✅

### Phase 1-6 实施前检查

**每个 Phase 开始前**:
- [ ] 重新阅读约束条件
- [ ] 检查依赖的 Core 模块 API 可用性
- [ ] 确认配置结构符合规范
- [ ] 确认测试计划符合约束

**每个 Phase 完成后**:
- [ ] 运行禁止项检查脚本
- [ ] 运行必需项检查脚本
- [ ] 运行单元测试（100% 通过）
- [ ] 代码审查（至少一人）
- [ ] 更新本检查清单

**最终提交前**:
- [ ] 所有检查点通过
- [ ] 所有测试通过
- [ ] 性能基准测试通过
- [ ] 文档更新完成
- [ ] 团队评审通过

---

## 7. 违规处理流程

### 发现违规时
1. **停止开发** - 立即停止当前工作
2. **记录违规** - 在本文档中记录违规内容
3. **分析原因** - 确定为何违反约束
4. **修正代码** - 按约束条件重构
5. **验证修正** - 重新运行检查清单
6. **继续开发** - 确认无违规后继续

### 违规记录
| 日期 | 文件 | 违规内容 | 状态 | 修正人 |
|------|------|---------|------|--------|
| - | - | - | - | - |

---

## 8. 快速检查脚本

### 一键检查脚本
```bash
#!/bin/bash
# check_constraints.sh

echo "========================================"
echo "Persistency 重构约束检查"
echo "========================================"

VIOLATIONS=0

# 检查禁止的文件系统库
echo "1. 检查禁止的文件系统库..."
if grep -r "std::filesystem\|boost::filesystem" modules/Persistency/source/ 2>/dev/null; then
    echo "  ❌ 发现禁止的文件系统库"
    VIOLATIONS=$((VIOLATIONS + 1))
else
    echo "  ✅ 未发现禁止的文件系统库"
fi

# 检查禁止的文件操作
echo "2. 检查禁止的文件操作..."
if grep -r "std::ofstream\|std::ifstream\|fopen\|fread" modules/Persistency/source/ 2>/dev/null; then
    echo "  ❌ 发现禁止的文件操作"
    VIOLATIONS=$((VIOLATIONS + 1))
else
    echo "  ✅ 未发现禁止的文件操作"
fi

# 检查 Core 类型使用
echo "3. 检查 Core 类型使用..."
if grep -q "Core::File::Util\|Core::Path::\|Core::ConfigManager" modules/Persistency/source/src/*.cpp; then
    echo "  ✅ 正在使用 Core 类型"
else
    echo "  ❌ 未使用 Core 类型"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 检查配置模块名
echo "4. 检查配置模块名..."
if grep -q 'getModuleConfigJson("persistency")' modules/Persistency/source/src/*.cpp; then
    echo "  ✅ 配置模块名正确"
else
    echo "  ⚠️  未找到配置加载代码"
fi

# 检查 Backend 纯粹性
echo "5. 检查 Backend 纯粹性..."
if grep -q "CReplicaManager" modules/Persistency/source/src/*Backend.cpp 2>/dev/null; then
    echo "  ❌ Backend 包含副本逻辑"
    VIOLATIONS=$((VIOLATIONS + 1))
else
    echo "  ✅ Backend 保持纯粹"
fi

# 总结
echo "========================================"
if [ $VIOLATIONS -eq 0 ]; then
    echo "✅ 所有约束检查通过"
    exit 0
else
    echo "❌ 发现 $VIOLATIONS 个违规项"
    exit 1
fi
```

**使用方法**:
```bash
cd /home/ddk/1_workspace/2_middleware/LightAP
chmod +x modules/Persistency/check_constraints.sh
./modules/Persistency/check_constraints.sh
```

---

## 附录: Core 模块 API 参考

### Core::File::Util
```cpp
static Bool exists(const String& path);
static Result<void> readAllBytes(const String& path, Vector<UInt8>& data);
static Result<void> writeAllBytes(const String& path, const UInt8* data, Size size);
static Result<void> remove(const String& path);
static Result<UInt64> getFileSize(const String& path);
```

### Core::Path
```cpp
static String append(const String& base, const String& relative);
static Result<void> createDirectory(const String& path);
static Bool isDirectory(const String& path);
static String getFileName(const String& path);
static String getFolder(const String& path);
static String getExtension(const String& path);
```

### Core::ConfigManager
```cpp
static ConfigManager& getInstance();
nlohmann::json getModuleConfigJson(const String& moduleName);
Result<void> setModuleConfig(const String& moduleName, const nlohmann::json& config);
```

### Core::Crypto
```cpp
static Crypto& getInstance();
Result<Vector<UInt8>> encrypt(const String& algorithm, const String& keyId, const Vector<UInt8>& plainData);
Result<Vector<UInt8>> decrypt(const String& algorithm, const String& keyId, const Vector<UInt8>& encryptedData);
Result<Vector<UInt8>> computeChecksum(const String& algorithm, const Vector<UInt8>& data);
```

---

**文档维护**:
- 每次重构实施前必须阅读
- 每次发现违规必须记录
- 每次 Core 模块 API 变更必须更新
- 每周检查一次合规性

**责任人**: 重构项目负责人  
**审核人**: 架构师 + 代码审查员
