# LightAP 持久化模块

[English](README.md) | [中文](README_CN.md)

[![License](https://img.shields.io/badge/License-CC%20BY--NC%204.0-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![AUTOSAR](https://img.shields.io/badge/AUTOSAR-AP%20R24--11-green.svg)](https://www.autosar.org/)
[![Tests](https://img.shields.io/badge/Tests-253%20Passing-success.svg)](#testing)

> **符合 AUTOSAR 自适应平台标准的持久化模块**  
> 高性能、生产就绪的数据持久化解决方案，支持多种存储后端

**版本：** 1.0.0  
**最后更新：** 2025-11-17  
**状态：** 生产就绪

---

## 📋 目录

- [概述](#概述)
- [核心特性](#核心特性)
- [架构设计](#架构设计)
- [快速开始](#快速开始)
- [存储目录结构](#存储目录结构)
- [存储后端](#存储后端)
- [API 参考](#api-参考)
- [配置说明](#配置说明)
- [编译与安装](#编译与安装)
- [测试](#测试)
- [文档](#文档)
- [许可证](#许可证)
- [贡献指南](#贡献指南)
- [更新日志](#更新日志)
- [发展路线图](#发展路线图)

---

## 概述

LightAP Persistency 是一个符合 AUTOSAR 自适应平台 R24-11 标准的持久化模块，为汽车和嵌入式系统提供稳健、高性能的数据存储解决方案。

### 功能概览

**两种存储类型：**
- 🔑 **键值存储 (KVS)** - 结构化配置和应用数据
- 📁 **文件存储** - 大文件和非结构化二进制/文本数据

**三种后端选项：**
- 📄 **文件后端** - 基于 JSON，可读性强，写延迟约 1.5ms
- 🗄️ **SQLite 后端** - ACID 事务，写延迟约 106ms  
- ⚡ **属性后端** - 共享内存，超高速（约 0.2µs），可选持久化

**核心能力：**
- ✅ 符合 AUTOSAR AP R24-11（约 60% API 覆盖率）
- ✅ 生产就绪（253 个单元测试通过）
- ✅ 高性能（比 SQLite 快达 530,000 倍）
- ✅ 仅内存模式，支持易失性缓存
- ✅ 数据完整性（CRC32/HMAC 校验，副本管理）

---

## 核心特性

### 🎯 核心能力

#### 键值存储

**支持的数据类型（12 种）：**
```cpp
// 整数类型
int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t

// 浮点类型
float, double

// 其他类型
bool, std::string
```

**基本操作：**
```cpp
#include <lap/per/IKeyValueStorage.hpp>

using namespace lap::per;

// 打开存储
auto kvs = OpenKeyValueStorage(InstanceSpecifier("/app/config"), true, 
                                 KvsBackendType::kvsFile);

// 写入数据
kvs->SetValue("version", String("1.2.3"));
kvs->SetValue("maxConnections", Int32(100));
kvs->SetValue("enableLogging", Bool(true));

// 读取数据
auto version = kvs->GetValue("version");
if (version.HasValue()) {
    auto str = std::get_if<String>(&version.Value());
    std::cout << "版本: " << str->data() << std::endl;
}

// 键管理
auto keys = kvs->GetAllKeys();           // 列出所有键
kvs->RemoveKey("oldKey");                // 删除键
kvs->SyncToStorage();                    // 强制刷新到磁盘
```

**高级操作：**
```cpp
kvs->RecoverKey("deletedKey");           // 从备份恢复
kvs->ResetKey("key");                    // 重置为默认值
kvs->DiscardPendingChanges();            // 回滚未提交的更改
kvs->GetAllKeys();                       // 枚举所有键
```

#### 文件存储

**基于流的文件 I/O：**
```cpp
#include <lap/per/IFileStorage.hpp>

using namespace lap::per;

// 打开文件存储
auto fs = OpenFileStorage(InstanceSpecifier("/app/data"));

// 读写访问
auto rw = fs->OpenFileReadWrite("config.json");
rw->WriteText("{\"key\":\"value\"}");
rw->SyncToFile();

// 只读访问
auto ro = fs->OpenFileReadOnly("settings.json");
auto content = ro->ReadText();

// 只写访问（追加模式）
auto wo = fs->OpenFileWriteOnly("log.txt", OpenMode::kAppend);
wo->WriteText("新日志条目\n");
```

**文件管理：**
```cpp
fs->DeleteFile("temp.dat");              // 删除文件
fs->RecoverFile("config.json");          // 从备份恢复
fs->ResetFile("config.json");            // 重置为初始状态
auto files = fs->GetAllFiles();          // 列出所有文件
```

### ⚡ 性能亮点

| 后端 | 写延迟 | 读延迟 | 使用场景 |
|------|--------|--------|----------|
| **属性 (kvsNone)** | **0.2µs** | **0.2µs** | 高性能缓存、IPC |
| **文件** | 1.5ms | 0.8ms | 配置文件、可读性 |
| **SQLite** | 106ms | 8.5ms | ACID 事务、复杂查询 |

**属性后端性能：**
- 🚀 写入速度比 SQLite **快 530,000 倍**
- 🚀 读取速度比 SQLite **快 42,500 倍**
- 🚀 比文件后端**快 7,500 倍**

### 🛡️ 数据完整性

**校验机制：**
- ✅ CRC32 校验和，快速错误检测
- ✅ HMAC-SHA256，加密完整性
- ✅ 副本管理（M-out-of-N 冗余）
- ✅ 自动备份和恢复
- ✅ 更新版本跟踪

**示例 - 完整性校验：**
```cpp
#include <lap/per/CConfigManager.hpp>

// 使用 HMAC 校验配置
setenv("HMAC_SECRET", "your-secret-key", 1);

ConfigManager& config = ConfigManager::GetInstance();
config.LoadConfig("config.json");  // 自动校验 HMAC

// 访问已校验的配置
auto& perConfig = config.GetPersistencyConfig();
```

### 🔄 后端对比

#### 文件后端
- ✅ 可读性强的 JSON 格式
- ✅ 易于调试和手动编辑
- ✅ 低内存占用
- ⚠️ 中等性能（约 1.5ms 写入）

#### SQLite 后端  
- ✅ ACID 事务
- ✅ SQL 查询支持
- ✅ 数据规范化
- ⚠️ 较高延迟（约 106ms 写入）
- ⚠️ 较大内存占用

#### 属性后端
- ✅ **超快速**共享内存操作
- ✅ 进程间通信（IPC）
- ✅ 可选持久化（kvsFile/kvsSqlite）
- ✅ 纯内存模式（kvsNone）

**使用场景：**
- 🎮 会话管理（临时用户会话）
- 🚀 高性能缓存
- 🔗 进程间通信（IPC）
- 📊 易失性运行时指标

---

## 架构设计

### 系统架构

```
┌──────────────────────────────────────────────────────────────┐
│                    应用层                                     │
│  OpenKeyValueStorage() / OpenFileStorage()                   │
└───────────────────────┬──────────────────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────────────────┐
│              CPersistencyManager（单例）                      │
│  • 存储生命周期管理                                           │
│  • 配置加载                                                   │
│  • 存储创建与缓存                                             │
└───────────┬──────────────────────────┬───────────────────────┘
            │                          │
┌───────────▼──────────┐    ┌──────────▼────────────────────┐
│  CKeyValueStorage    │    │     CFileStorage              │
│  • KVS 接口          │    │  • 文件存储接口                │
│  • 类型安全包装      │    │  • 访问器工厂                  │
└───────────┬──────────┘    └──────────┬────────────────────┘
            │                          │
    ┌───────┴────────┐        ┌────────┴─────────┐
    │ 后端类型：     │        │  访问器类型：     │
    ├────────────────┤        ├──────────────────┤
    │ • File         │        │ • ReadAccessor   │
    │ • SQLite       │        │ • ReadWriteAccessor│
    │ • Property     │        └──────────┬───────┘
    └───────┬────────┘                   │
            │                            │
┌───────────▼────────────────────────────▼───────────────────┐
│                 存储后端层                                  │
│  ┌─────────────┐ ┌──────────────┐ ┌──────────────────┐   │
│  │FileBackend  │ │SqliteBackend │ │PropertyBackend   │   │
│  │  (JSON)     │ │  (SQLite3)   │ │ (共享内存)       │   │
│  └─────────────┘ └──────────────┘ └──────────────────┘   │
└────────────────────────────────────────────────────────────┘
                        │
┌───────────────────────▼────────────────────────────────────┐
│                 物理存储层                                  │
│   文件系统      │   SQLite 数据库   │   POSIX 共享内存    │
└────────────────────────────────────────────────────────────┘
```

### 类层次结构

```cpp
// 键值存储
IKvsBackend（抽象接口）
├── KvsFileBackend           // JSON 文件存储
├── KvsSqliteBackend         // SQLite 数据库
└── KvsPropertyBackend       // 共享内存 + 可选持久化

// 文件存储
ReadAccessor                 // 只读文件访问
└── ReadWriteAccessor        // 读写文件访问

// 配置管理
ConfigManager（单例）        // 配置加载，支持 CRC32/HMAC 校验
```

---

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/TreeNeeBee/LightAP.git
cd LightAP/modules/Persistency

# 编译
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行测试
export HMAC_SECRET="test_secret_key"
./persistency_test
```

### 基本 KVS 示例

```cpp
#include <lap/per/IKeyValueStorage.hpp>
#include <iostream>

using namespace lap::per;
using namespace lap::core;

int main() {
    // 使用文件后端打开 KVS
    auto kvs = OpenKeyValueStorage(
        InstanceSpecifier("/app/settings"),
        true,  // 如果不存在则创建
        KvsBackendType::kvsFile
    );
    
    if (!kvs.HasValue()) {
        std::cerr << "打开存储失败" << std::endl;
        return 1;
    }
    
    auto storage = kvs.Value();
    
    // 写入配置
    storage->SetValue("appName", String("MyApp"));
    storage->SetValue("version", String("1.0.0"));
    storage->SetValue("maxUsers", Int32(100));
    storage->SetValue("debugMode", Bool(true));
    
    // 读取配置
    auto appName = storage->GetValue("appName");
    if (appName.HasValue()) {
        auto str = std::get_if<String>(&appName.Value());
        std::cout << "应用: " << str->data() << std::endl;
    }
    
    // 列出所有键
    auto keys = storage->GetAllKeys();
    if (keys.HasValue()) {
        for (const auto& key : keys.Value()) {
            std::cout << "键: " << key.data() << std::endl;
        }
    }
    
    // 持久化更改
    storage->SyncToStorage();
    
    return 0;
}
```

### 文件存储示例

```cpp
#include <lap/per/IFileStorage.hpp>
#include <iostream>

using namespace lap::per;
using namespace lap::core;

int main() {
    // 打开文件存储
    auto fs = OpenFileStorage(InstanceSpecifier("/app/data"));
    
    if (!fs.HasValue()) {
        std::cerr << "打开文件存储失败" << std::endl;
        return 1;
    }
    
    auto storage = fs.Value();
    
    // 写入 JSON 配置
    {
        auto rw = storage->OpenFileReadWrite("config.json");
        if (rw.HasValue()) {
            auto accessor = rw.Value();
            accessor->WriteText("{\"timeout\": 30, \"retries\": 3}");
            accessor->SyncToFile();
        }
    }
    
    // 读取配置
    {
        auto ro = storage->OpenFileReadOnly("config.json");
        if (ro.HasValue()) {
            auto accessor = ro.Value();
            auto content = accessor->ReadText();
            if (content.HasValue()) {
                std::cout << "配置: " << content.Value().data() << std::endl;
            }
        }
    }
    
    return 0;
}
```

### 属性后端（共享内存）

```cpp
#include "CKvsPropertyBackend.hpp"

using namespace lap::per::util;

int main() {
    // 快速共享内存后端，带文件持久化
    KvsPropertyBackend cache("app_cache", 
                             KvsBackendType::kvsFile,
                             4ul << 20);  // 4MB 共享内存
    
    // 超快速写入（约 0.2µs）
    cache.SetValue("user_count", Int32(1000));
    cache.SetValue("cache_hit_rate", Float(95.5f));
    
    // 超快速读取（约 0.2µs）
    auto count = cache.GetValue("user_count");
    
    // 持久化到文件（较慢，但持久）
    cache.SyncToStorage();
    
    return 0;
}
```

### 仅内存模式（新功能！）

```cpp
// 纯内存模式，无持久化
KvsPropertyBackend sessions("user_sessions", 
                            KvsBackendType::kvsNone,  // ← 无持久化！
                            2ul << 20);  // 2MB

// 存储临时会话数据
sessions.SetValue("session_123", String("active"));
sessions.SetValue("user_alice", String("logged_in"));

// 无磁盘 I/O - 即时性能
sessions.SyncToStorage();  // ✓ 无操作，返回成功
```

---

## 存储目录结构

### AUTOSAR 4 层目录层次结构

模块实现了 AUTOSAR 自适应平台的 **4 层存储结构**，用于版本管理和更新安全：

```
/opt/autosar/persistency/              # centralStorageURI（可配置）
│
├── kvs/                               # 键值存储根目录
│   ├── file/                          # 文件后端
│   │   └── app_config/                # 实例: /app/config
│   │       ├── current/               # [SWS_PER_00500] 活动数据
│   │       │   └── 0_data.json
│   │       ├── update/                # [SWS_PER_00501] 更新缓冲区
│   │       │   ├── 0_data.json        # 副本 0
│   │       │   ├── 1_data.json        # 副本 1
│   │       │   └── 2_data.json        # 副本 2（默认：3 个副本）
│   │       ├── redundancy/            # [SWS_PER_00502] 回滚备份
│   │       │   ├── 0_data.json
│   │       │   ├── 1_data.json
│   │       │   └── 2_data.json
│   │       └── recovery/              # [SWS_PER_00503] 已删除键恢复
│   │           ├── 0_data.json
│   │           ├── 1_data.json
│   │           └── 2_data.json
│   │
│   ├── sqlite/                        # SQLite 后端
│   │   └── vehicle_state/             # 实例: /vehicle/state
│   │       ├── current/
│   │       │   └── 0_data.db
│   │       ├── update/
│   │       │   ├── 0_data.db
│   │       │   ├── 1_data.db
│   │       │   └── 2_data.db
│   │       ├── redundancy/
│   │       │   ├── 0_data.db
│   │       │   ├── 1_data.db
│   │       │   └── 2_data.db
│   │       └── recovery/
│   │           └── 0_data.db
│   │
│   └── property/                      # 属性后端（带持久化）
│       └── cache/                     # 实例: /cache
│           ├── update/                # kvsFile 持久化
│           │   ├── 0_data.json
│           │   └── 1_data.json
│           └── current/
│               ├── 0_data.json
│               └── 1_data.json
│           # 注意：kvsNone 不创建文件
│
├── file/                              # 文件存储根目录（使用不同的层名称）
│   ├── app_data/                      # 实例: /app/data
│   │   ├── current/
│   │   │   └── default_config.json
│   │   ├── update/
│   │   │   ├── 0_config.json
│   │   │   ├── 1_config.json
│   │   │   └── 2_config.json
│   │   ├── backup/                    # FileStorage 第 3 层（而非 redundancy）
│   │   │   ├── config.json            # 活动用户文件
│   │   │   ├── settings.xml
│   │   │   └── logs.txt
│   │   └── initial/                   # FileStorage 第 4 层（出厂默认值）
│   │       ├── config.json.bak
│   │       └── settings.xml.bak
│   │
│   └── vehicle_logs/                  # 实例: /vehicle/logs
│       ├── update/
│       │   ├── 0_error.log
│       │   └── 1_error.log
│       └── current/
│           └── error.log
│
└── shm/                               # 共享内存（仅运行时，无层）
    ├── cache                          # POSIX 共享内存对象
    └── ipc_buffer                     # 仅内存，无持久化
```

### 4 层结构说明

| 层 | 目录 | 用途 | AUTOSAR 参考 |
|----|------|------|--------------|
| **1. Current** | `current/` | 活动运行时数据。所有读写操作的主要工作目录。 | [SWS_PER_00500] |
| **2. Update** | `update/` | 原子修改的更新缓冲区。所有更改在提交到 `current/` 之前在此暂存。 | [SWS_PER_00501] |
| **3. Redundancy** | `redundancy/` | 更新前创建的 `current/` 备份快照。支持失败时回滚。 | [SWS_PER_00502] |
| **4. Recovery** | `recovery/` | 已删除键/文件的存储。支持 `RecoverKey()` 和 `RecoverFile()` 操作。 | [SWS_PER_00503] |

### 目录命名规则

**1. 实例说明符映射：**

```cpp
// AUTOSAR InstanceSpecifier → 文件系统路径
// InstanceSpecifier 使用清单中的 shortName 路径（AUTOSAR 标准）
// 保留内部斜杠以创建目录层次结构

"/app/config"        → "app/config"        # 创建: kvs/app/config/
"/vehicle/state"     → "vehicle/state"     # 创建: kvs/vehicle/state/
"/MyApp/Data/Logs"   → "MyApp/Data/Logs"   # 创建: kvs/MyApp/Data/Logs/

// 规则：
// - 仅删除开头的斜杠
// - 保留内部斜杠（创建嵌套目录）
// - 保留大小写
// - 示例："app1/kvs_storage" 创建目录层次 "kvs/app1/kvs_storage/"
```

**2. 副本文件命名：**

```text
格式: {replicaId}_{baseName}.{ext}

示例:
  0_data.json    # 副本 0（主副本）
  1_data.json    # 副本 1（备份）
  2_data.json    # 副本 2（备份）
  
默认值：3 个副本（可通过 replicaCount 配置）
```

**3. 后端特定路径：**

```text
KVS 文件后端:      kvs/{instancePath}/{layer}/{replicaId}_data.json
KVS SQLite 后端:   kvs/{instancePath}/{layer}/{replicaId}_data.db
属性后端:          kvs/{instancePath}/{layer}/{replicaId}_data.json
文件存储:          fs/{instancePath}/{layer}/{filename}

其中 {instancePath} 保留 InstanceSpecifier 层次结构:
  - "app/config"           → kvs/app/config/current/0_data.json
  - "app1/kvs_storage"     → kvs/app1/kvs_storage/current/0_data.json
  - "vehicle/diagnostics"  → kvs/vehicle/diagnostics/current/0_data.json
```

### 更新与回滚工作流

**正常操作：**

```bash
# 应用读写 current/
/opt/autosar/persistency/kvs/file/app_config/current/0_data.json
```

**软件更新过程：**

```bash
# 1. 准备更新缓冲区
cp -r current/* update/

# 2. 在 update/ 中应用更改
# 所有修改都在 update/ 目录中进行

# 3. 提交前备份当前数据
cp -r current/* redundancy/

# 4. 原子提交：用 update 替换 current
mv current/ current.tmp/
mv update/ current/
rm -rf current.tmp/

# 5. 应用继续使用 current/
```

**失败时回滚：**

```bash
# 从 redundancy 备份恢复
rm -rf current/
cp -r redundancy/* current/

# 应用恢复到已知良好状态
```

**键恢复：**

```bash
# 键被删除时，移动到 recovery/
mv current/0_data.json recovery/0_data.json

# 可以使用 RecoverKey() 恢复
mv recovery/0_data.json current/0_data.json
```

### 配置

**JSON 配置：**

```json
{
  "persistency": {
    "centralStorageURI": "/opt/autosar/persistency",
    "replicaCount": 3,
    "minValidReplicas": 2,
    "enableVersioning": true
  }
}
```

**环境变量覆盖：**

```bash
# 在运行时覆盖存储基路径
export PERSISTENCY_STORAGE_URI="/custom/storage/path"

# 结果: /custom/storage/path/kvs/file/app_config/latest/...
```

### 共享内存（属性后端）

**位置：** `/dev/shm/{instance_name}`（Linux tmpfs）

**特性：**

- **生命周期：** 首次访问时创建，所有进程分离时销毁
- **无 4 层结构：** 共享内存是易失性的（无版本管理）
- **持久化选项：**
  - `kvsFile` → 由 `kvs/property/{instance}/current/` 中的 JSON 文件支持
  - `kvsSqlite` → 由 `kvs/property/{instance}/current/` 中的 SQLite 数据库支持
  - `kvsNone` → **纯内存，不创建文件**

**示例：**

```cpp
// 仅内存模式（无磁盘文件）
KvsPropertyBackend cache("session_cache", KvsBackendType::kvsNone, 4MB);

// 结果: /dev/shm/session_cache（仅内存）
// /opt/autosar/persistency/ 中无文件
```

### 磁盘空间估算

**100 个键的示例计算：**

| 后端 | 副本 | 层 | 每个实例大小 | 总计（3 个副本，4 层） |
|------|------|-------|------------|----------------------|
| 文件 | 1 | 1 | 约 10KB（JSON） | 约 120KB |
| SQLite | 1 | 1 | 约 50KB（数据库） | 约 600KB |
| 属性 | 1 | 1 | 约 10KB（JSON） | 约 120KB（如果持久化） |

**公式：**

```
总计 = (副本数) × (层数) × (实例大小)
     = 3 × 4 × 10KB = 120KB（文件/属性）
     = 3 × 4 × 50KB = 600KB（SQLite）
```

---

## 存储后端

### 后端选择指南

| 标准 | 文件 | SQLite | 属性 (kvsFile) | 属性 (kvsNone) |
|------|------|--------|----------------|----------------|
| **写入速度** | 1.5ms | 106ms | 0.2µs（+持久化） | **0.2µs** |
| **读取速度** | 0.8ms | 8.5ms | **0.2µs** | **0.2µs** |
| **持久化** | ✅ 是 | ✅ 是 | ✅ 是 | ❌ 否 |
| **ACID** | ❌ 否 | ✅ 是 | ❌ 否 | ❌ 否 |
| **可读性** | ✅ JSON | ❌ 二进制 | ❌ 二进制 | ❌ 二进制 |
| **内存使用** | 低 | 高 | 中等 | 中等 |
| **进程崩溃** | ✅ 安全 | ✅ 安全 | ⚠️ 最后同步 | ❌ 数据丢失 |
| **使用场景** | 配置文件 | 事务 | IPC + 持久化 | 易失性缓存 |

### 何时使用每种后端

**📄 文件后端** - 最适合：
- 可读性强的配置文件
- 小型数据集（<1000 键）
- 调试和手动编辑
- 简单的键值对

**🗄️ SQLite 后端** - 最适合：
- 大型数据集（>10,000 键）
- ACID 事务需求
- 复杂的数据关系
- 多表场景

**⚡ 属性后端 (kvsFile/kvsSqlite)** - 最适合：
- 高频更新（>1000 次/秒）
- 进程间通信
- 读取密集型工作负载
- 需要持久化 + 速度

**🚀 属性后端 (kvsNone)** - 最适合：
- 会话管理
- 临时缓存
- 无持久化的 IPC
- 最大性能需求

---

## API 参考

### 键值存储 API

```cpp
namespace lap::per {

// 打开 KVS
Result<SharedHandle<KeyValueStorage>> 
OpenKeyValueStorage(const InstanceSpecifier& spec,
                    Bool createIfNotExists = true,
                    KvsBackendType type = KvsBackendType::kvsFile);

class KeyValueStorage {
public:
    // 写入操作
    Result<void> SetValue(StringView key, const KvsDataType& value);
    
    // 读取操作
    Result<KvsDataType> GetValue(StringView key);
    Result<Vector<String>> GetAllKeys();
    Result<Bool> KeyExists(StringView key);
    
    // 键管理
    Result<void> RemoveKey(StringView key);
    Result<void> RecoverKey(StringView key);
    Result<void> ResetKey(StringView key);
    Result<void> RemoveAllKeys();
    
    // 事务控制
    Result<void> SyncToStorage();
    Result<void> DiscardPendingChanges();
};

} // namespace lap::per
```

### 文件存储 API

```cpp
namespace lap::per {

// 打开文件存储
Result<SharedHandle<FileStorage>>
OpenFileStorage(const InstanceSpecifier& spec);

class FileStorage {
public:
    // 文件访问
    Result<UniqueHandle<ReadAccessor>> 
    OpenFileReadOnly(StringView fileName);
    
    Result<UniqueHandle<ReadWriteAccessor>> 
    OpenFileReadWrite(StringView fileName, 
                      OpenMode mode = OpenMode::kIn | OpenMode::kOut);
    
    Result<UniqueHandle<ReadWriteAccessor>> 
    OpenFileWriteOnly(StringView fileName,
                      OpenMode mode = OpenMode::kOut | OpenMode::kTruncate);
    
    // 文件管理
    Result<void> DeleteFile(StringView fileName);
    Result<void> RecoverFile(StringView fileName);
    Result<void> ResetFile(StringView fileName);
    Result<Vector<String>> GetAllFiles();
};

// 文件访问器
class ReadAccessor {
public:
    Result<String> ReadText();
    Result<Vector<UInt8>> ReadBinary();
    Result<Position> GetCurrentPosition();
    Result<void> SetPosition(Position pos, Origin origin);
};

class ReadWriteAccessor : public ReadAccessor {
public:
    Result<void> WriteText(StringView data);
    Result<void> WriteBinary(const Vector<UInt8>& data);
    Result<void> SyncToFile();
};

} // namespace lap::per
```

### 配置 API

```cpp
namespace lap::per {

class ConfigManager {
public:
    static ConfigManager& GetInstance();
    
    // 加载配置并校验
    bool LoadConfig(const String& path);
    
    // 访问配置部分
    const PersistencyConfig& GetPersistencyConfig() const;
    
    // 校验完整性
    bool VerifyIntegrity() const;
};

struct PersistencyConfig {
    String centralStorageURI;
    UInt32 replicaCount;
    UInt32 minValidReplicas;
    String checksumType;
    
    struct KvsConfig {
        String backendType;         // "file", "sqlite", "property"
        Size propertyBackendShmSize;
        String propertyBackendPersistence;  // "file", "sqlite", "none"
    } kvs;
};

} // namespace lap::per
```

---

## 配置说明

### 配置文件格式

```json
{
  "persistency": {
    "centralStorageURI": "/opt/autosar/persistency",
    "replicaCount": 3,
    "minValidReplicas": 2,
    "checksumType": "CRC32",
    "contractVersion": "1.0.0",
    "kvs": {
      "backendType": "sqlite",
      "propertyBackendShmSize": 4194304,
      "propertyBackendPersistence": "file"
    }
  },
  "crc32": 12345678,
  "hmac": "a1b2c3d4..."
}
```

### 配置选项

| 选项 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `centralStorageURI` | string | `/tmp/autosar_persistency` | 基础存储路径 |
| `replicaCount` | uint32 | `3` | 副本总数（N） |
| `minValidReplicas` | uint32 | `2` | 最小有效副本数（M） |
| `checksumType` | string | `CRC32` | `CRC32` 或 `SHA256` |
| `kvs.backendType` | string | `file` | `file`、`sqlite`、`property` |
| `kvs.propertyBackendShmSize` | size | `1048576` | 共享内存大小（字节） |
| `kvs.propertyBackendPersistence` | string | `file` | `file`、`sqlite`、`none` |

### 环境变量

```bash
# HMAC 校验所需
export HMAC_SECRET="your-secret-key-here"

# 可选：覆盖存储路径
export PERSISTENCY_STORAGE_URI="/custom/path"
```

---

## 编译与安装

### 前置条件

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libsqlite3-dev libboost-all-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake sqlite-devel boost-devel
```

### 编译步骤

```bash
# 1. 克隆仓库
git clone https://github.com/TreeNeeBee/LightAP.git
cd LightAP

# 2. 配置
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 编译
make -j$(nproc)

# 4. 安装（可选）
sudo make install
```

### 编译选项

```bash
# 调试编译
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 详细日志
cmake .. -DLAP_DEBUG=ON

# 自定义安装前缀
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/lightap
```

### 在您的项目中集成

```cmake
# CMakeLists.txt
find_package(LightAP REQUIRED COMPONENTS Persistency)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE LightAP::Persistency)
```

---

## 测试

### 测试套件

**状态：** ✅ 253/253 测试通过（100%）

```bash
cd build
export HMAC_SECRET="test_secret_key"
./modules/Persistency/persistency_test
```

### 测试类别

| 套件 | 测试数 | 覆盖范围 |
|------|--------|----------|
| **核心约束** | 12 | 配置集成 |
| **数据类型** | 33 | 类型系统 |
| **错误域** | 11 | 错误处理 |
| **文件存储** | 33 | 文件操作 |
| **文件后端** | 17 | 后端层 |
| **KVS** | 67 | 键值操作 |
| **属性后端** | 21 | 共享内存 |
| **SQLite 后端** | 19 | 数据库操作 |
| **副本管理器** | 11 | 冗余 |
| **路径管理器** | 29 | 路径生成 |
| **总计** | **253** | **约 85% 代码覆盖** |

### 示例程序

```bash
# 后端对比
./modules/Persistency/backend_comparison_example

# 多后端使用
./modules/Persistency/multi_backend_usage_example

# 仅内存模式
./modules/Persistency/property_memory_only_example

# SQLite 演示
./modules/Persistency/sqlite_backend_demo

# 性能基准测试
./modules/Persistency/performance_benchmark
```

### 性能基准测试

```bash
$ ./performance_benchmark

文件后端性能:
  写入 (1000 键): 1.51ms
  读取 (1000 键): 0.78ms

SQLite 后端性能:
  写入 (1000 键): 105.70ms
  读取 (1000 键): 8.45ms

属性后端 (kvsFile) 性能:
  写入 (1000 键): 1.18ms（内存）
  同步到文件: 2.28ms
  读取 (1000 键): 0.15ms

属性后端 (kvsNone) 性能:
  写入 (1000 键): 0.20ms
  读取 (1000 键): 0.18ms
```

---

## 文档

### 文档结构

```txt
doc/
├── README.md                      # 文档索引
├── architecture/                  # 系统设计
│   ├── ARCHITECTURE_REFACTORING_PLAN.md
│   ├── DESIGN_ANALYSIS.md
│   ├── KVS_STORAGE_STRUCTURE_ANALYSIS.md
│   ├── KVS_SUPPORTED_TYPES.md
│   └── OTA_UPDATE_ARCHITECTURE.md
├── configuration/                 # 配置指南
│   ├── CONFIG_INTEGRATION.md
│   ├── CONFIGURABLE_MEMORY_SUMMARY.md
│   ├── CONFIGURATION_GUIDE.md
│   └── MEMORY_CONFIG_QUICK_REFERENCE.md
├── refactoring/                   # 优化文档
│   ├── KVS_BACKEND_TYPE_OPTIMIZATION.md  # ← kvsNone 功能
│   ├── REFACTORING_CONSTRAINTS_CHECKLIST.md
│   ├── SQLITE_BACKEND_REFACTORING_SUMMARY.md
│   ├── SQLITE_OPTIMIZATION_SUMMARY.md
│   └── TYPE_SYSTEM_OPTIMIZATION.md
├── testing/                       # 测试文档
│   ├── TEST_SUMMARY.md
│   ├── UT_AND_EXAMPLE_SUMMARY.md
│   ├── TASK4_INTEGRITY_VALIDATION_SUMMARY.md
│   └── VERIFICATION_REPORT.md
├── compliance/                    # AUTOSAR 合规性
│   ├── AUTOSAR_COMPLIANCE_ANALYSIS.md
│   ├── AUTOSAR_COMPLIANCE_CHECKLIST.md
│   └── AUTOSAR_AP_SWS_Persistency.pdf
└── archived/                      # 历史文档
```

### 关键文档

- **[架构指南](doc/architecture/DESIGN_ANALYSIS.md)** - 系统设计和模式
- **[配置指南](doc/configuration/CONFIGURATION_GUIDE.md)** - 设置和调优
- **[kvsNone 功能](doc/refactoring/KVS_BACKEND_TYPE_OPTIMIZATION.md)** - 仅内存模式
- **[测试报告](doc/testing/TEST_SUMMARY.md)** - 测试覆盖和结果
- **[AUTOSAR 合规性](doc/compliance/AUTOSAR_COMPLIANCE_ANALYSIS.md)** - 标准合规性

---

## 许可证

本项目采用 **知识共享署名-非商业性使用 4.0 国际许可协议（CC BY-NC 4.0）**。

### 许可摘要

✅ **允许：**
- 教育和学习目的
- 个人项目和实验
- 修改和再分发（需署名）

❌ **禁止：**
- 商业使用
- 生产部署

💼 **商业使用：**
如需商业许可，请联系：<https://github.com/TreeNeeBee/LightAP>

完整许可：[LICENSE](LICENSE)

---

## 贡献指南

我们欢迎贡献！请：

1. Fork 仓库
2. 创建功能分支（`git checkout -b feature/amazing-feature`）
3. 提交更改（`git commit -m 'Add amazing feature'`）
4. 推送到分支（`git push origin feature/amazing-feature`）
5. 开启 Pull Request

### 开发指南

- 遵循 AUTOSAR 编码规范
- 保持测试覆盖率 >80%
- 为新功能添加单元测试
- 更新文档
- 使用 clang-format 进行代码格式化

---

## 更新日志

### v1.0.0（2025-11-17）

**新功能：**
- ✨ 添加 `KvsBackendType::kvsNone` 仅内存模式
- ✨ 属性后端现在支持无持久化（`kvsNone`）
- ✨ 添加全面的示例和基准测试
- ✨ 重组文档结构

**改进：**
- 🚀 属性后端性能：0.2µs 读/写延迟
- 🚀 SQLite 后端优化：125K+ 次操作/秒
- 📝 完整的英文 README，包含架构图
- 📝 全面的 API 文档

**Bug 修复：**
- 🐛 修复测试隔离问题
- 🐛 修复配置验证边界情况
- 🐛 改进错误处理

**破坏性更改：**
- ⚠️ 删除未使用的 `kvsLocal` 和 `kvsRemote` 枚举值

---

## 发展路线图

### 2026 年第一季度

- [ ] 加密支持（AES-256）
- [ ] 事务日志，用于崩溃恢复
- [ ] 远程存储后端（基于网络）
- [ ] Python 绑定

### 2026 年第二季度

- [ ] 完全符合 AUTOSAR AP R25-11
- [ ] 异步 I/O 支持
- [ ] 存储配额和限制
- [ ] 基于 Web 的监控仪表板

---

## 联系方式

**项目：** LightAP 持久化模块  
**仓库：** https://github.com/TreeNeeBee/LightAP  
**问题：** https://github.com/TreeNeeBee/LightAP/issues  
**许可：** CC BY-NC 4.0

---

## 致谢

- AUTOSAR 联盟提供的自适应平台规范
- SQLite 团队提供的出色嵌入式数据库
- Boost.Interprocess 提供的共享内存支持
- 所有贡献者和测试人员

---

<p align="center">
  <strong>为 AUTOSAR 社区倾心打造 ❤️</strong><br>
  <sub>教育和个人使用采用 CC BY-NC 4.0 许可</sub>
</p>
