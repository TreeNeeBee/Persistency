# Persistency 配置项完整说明

## 配置文件位置

推荐配置文件位置：
- 开发环境：`config.json` (工作目录)
- 生产环境：`/etc/lightap/persistency_config.json`
- 自定义：通过 `ConfigManager::initialize(path)` 指定

## 核心配置项

### 1. Property Backend 共享内存配置

**配置键：** `kvs.propertyBackendShmSize`  
**类型：** 整数（字节）  
**默认值：** 1048576 (1MB)  
**范围：** 524288 (512KB) ~ 67108864 (64MB)

```json
{
  "kvs": {
    "propertyBackendShmSize": 16777216
  }
}
```

**推荐值：**
- IoT设备：524288 (512KB)
- 嵌入式应用：1048576 (1MB)
- 标准应用：4194304 (4MB)
- 服务器应用：16777216 (16MB)
- 高性能缓存：33554432 (32MB) 或更大

### 2. Property Backend 持久化类型

**配置键：** `kvs.propertyBackendPersistence`  
**类型：** 字符串  
**默认值：** "file"  
**可选值：** "file" | "sqlite"

```json
{
  "kvs": {
    "propertyBackendPersistence": "file"
  }
}
```

**选择建议：**
- **file**: 简单、快速、单进程场景
- **sqlite**: ACID保证、多进程支持、可查询

### 3. Backend 类型选择

**配置键：** `kvs.backendType`  
**类型：** 字符串  
**默认值：** "file"  
**可选值：** "file" | "sqlite" | "property"

```json
{
  "kvs": {
    "backendType": "property"
  }
}
```

**性能对比：**
| Backend | 读性能 | 写性能 | 适用场景 |
|---------|--------|--------|---------|
| file | 中等 (0.5ms) | 中等 (1ms) | 简单配置存储 |
| sqlite | 慢 (9ms) | 最慢 (118ms) | 需要ACID保证 |
| property | **最快 (0.15ms)** | 快 (1ms) | 高频读写、IPC |

### 4. 中央存储配置

**配置键：** `centralStorageURI`  
**类型：** 字符串  
**默认值：** "/tmp/autosar_persistency"

```json
{
  "centralStorageURI": "/var/lib/lightap/persistency"
}
```

### 5. 冗余和一致性配置

```json
{
  "replicaCount": 3,
  "minValidReplicas": 2,
  "checksumType": "CRC32",
  "redundancyHandling": "KEEP_REDUNDANCY",
  "updateStrategy": "KEEP_LAST_VALID"
}
```

## 完整配置示例

### 示例1: IoT设备配置 (最小资源)

```json
{
  "centralStorageURI": "/data/persistency",
  "replicaCount": 1,
  "minValidReplicas": 1,
  "checksumType": "CRC32",
  
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 524288,
    "propertyBackendPersistence": "file"
  }
}
```

**特点：**
- 512KB共享内存
- 单副本（节省存储）
- File持久化（快速）
- 适合资源受限设备

### 示例2: 边缘网关配置 (平衡性能)

```json
{
  "centralStorageURI": "/var/lib/gateway/persistency",
  "replicaCount": 2,
  "minValidReplicas": 2,
  "checksumType": "SHA256",
  
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 8388608,
    "propertyBackendPersistence": "file"
  }
}
```

**特点：**
- 8MB共享内存
- 双副本（可靠性）
- SHA256校验（安全）
- 适合边缘计算场景

### 示例3: 服务器应用配置 (高性能)

```json
{
  "centralStorageURI": "/var/lib/lightap/persistency",
  "replicaCount": 3,
  "minValidReplicas": 2,
  "checksumType": "SHA256",
  
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 33554432,
    "propertyBackendPersistence": "sqlite"
  }
}
```

**特点：**
- 32MB共享内存（大容量）
- 三副本（高可靠）
- SQLite持久化（ACID）
- 适合服务器环境

## 代码中使用配置

### 方式1: 通过ConfigManager加载

```cpp
#include "CConfigManager.hpp"
#include "CPersistencyManager.hpp"

// 加载配置
lap::per::util::ConfigManager::initialize("persistency_config.json");

// 创建PersistencyManager（自动读取配置）
auto manager = lap::per::CPersistencyManager::GetInstance();
```

### 方式2: 直接读取配置值

```cpp
#include "CConfigManager.hpp"
#include "CKvsPropertyBackend.hpp"

auto& config = lap::per::util::ConfigManager::getInstance();

// 读取共享内存大小
auto shmSizeOpt = config.get<size_t>("kvs.propertyBackendShmSize");
size_t shmSize = shmSizeOpt.value_or(1ul << 20); // 默认1MB

// 读取持久化类型
auto persistTypeStr = config.get<std::string>("kvs.propertyBackendPersistence");
auto persistType = (persistTypeStr == "sqlite") 
    ? KvsBackendType::kvsSqlite 
    : KvsBackendType::kvsFile;

// 创建backend
KvsPropertyBackend backend("my_app", persistType, shmSize);
```

### 方式3: 运行时更新配置

```cpp
auto& config = lap::per::util::ConfigManager::getInstance();

// 更新共享内存大小为16MB
config.set("kvs.propertyBackendShmSize", 16777216);

// 更新持久化类型
config.set("kvs.propertyBackendPersistence", "sqlite");

// 保存配置
config.saveToFile("persistency_config.json");
```

## 配置验证

### 自动验证规则

ConfigManager会自动验证：
1. **共享内存大小范围：** 512KB ~ 64MB
2. **持久化类型：** 必须是 "file" 或 "sqlite"
3. **Backend类型：** 必须是有效值
4. **路径合法性：** 存储路径必须可访问

### 手动验证

```cpp
auto& config = ConfigManager::getInstance();

// 检查配置是否存在
if (!config.has("kvs.propertyBackendShmSize")) {
    std::cerr << "Missing configuration: propertyBackendShmSize" << std::endl;
}

// 验证范围
auto shmSize = config.get<size_t>("kvs.propertyBackendShmSize").value();
if (shmSize < 524288 || shmSize > 67108864) {
    std::cerr << "Invalid shared memory size: " << shmSize << std::endl;
}
```

## 配置优先级

1. **命令行参数** (如果支持)
2. **环境变量** `LIGHTAP_PERSISTENCY_CONFIG`
3. **配置文件** (通过initialize指定)
4. **默认值** (硬编码在代码中)

## 性能调优建议

### 根据使用模式调整

**读密集型场景：**
```json
{
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 16777216,
    "propertyBackendPersistence": "file"
  }
}
```
- 大共享内存
- File持久化（快速加载）

**写密集型场景：**
```json
{
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 8388608,
    "propertyBackendPersistence": "sqlite"
  }
}
```
- 适中共享内存
- SQLite持久化（事务支持）

**混合场景：**
```json
{
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 16777216,
    "propertyBackendPersistence": "sqlite"
  }
}
```
- 大共享内存（快速读）
- SQLite持久化（可靠写）

## 故障排查

### 问题1: boost::interprocess::bad_alloc

**原因：** 共享内存不足  
**解决：** 增加 `propertyBackendShmSize`

```json
{
  "kvs": {
    "propertyBackendShmSize": 33554432  // 增加到32MB
  }
}
```

### 问题2: 性能下降

**检查：**
1. 共享内存是否过小导致频繁换页
2. 持久化后端是否匹配使用场景
3. 是否启用了不必要的加密/压缩

**优化：**
```json
{
  "kvs": {
    "propertyBackendShmSize": 16777216,  // 增加内存
    "propertyBackendPersistence": "file"  // 简化持久化
  }
}
```

### 问题3: 配置不生效

**检查清单：**
- [ ] 配置文件路径正确
- [ ] JSON格式正确（使用验证工具）
- [ ] 配置键名称正确
- [ ] 配置值类型正确
- [ ] 应用重启（配置更改需要重启）

## 监控和日志

### 启用配置日志

```json
{
  "Monitoring": {
    "LogLevel": "INFO",
    "EnablePerformanceLogging": true
  }
}
```

### 日志输出示例

```
[INFO] KvsPropertyBackend initialized with SHM name: shm_kvs_12345_my_app, 
       identifier: my_app, size: 16384 KB
[INFO] Property backend using File backend for persistence
[INFO] Loading 1234 keys from persistence backend
```

## 最佳实践

1. ✅ **使用配置文件而非硬编码**
2. ✅ **根据实际数据量调整共享内存**
3. ✅ **测试环境验证配置后再部署**
4. ✅ **监控内存使用并适时调整**
5. ✅ **为不同环境准备不同配置**
6. ✅ **定期备份配置文件**
7. ✅ **使用版本控制管理配置**

## 参考示例

查看完整示例：
```bash
cd modules/Persistency/test/examples
./config_driven_property_backend_example
```

查看配置模板：
```bash
cat modules/Persistency/test/examples/persistency_full_config.json
```
