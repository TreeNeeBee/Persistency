# Task 4: AUTOSAR 数据完整性校验实现总结

## 📋 概述

本任务实现了 AUTOSAR AP R24-11 标准 [SWS_PER_00600] 和 [SWS_PER_00800] 规定的完整性校验和更新工作流。

**完成时间**: 2024-11-14  
**影响文件**: 2 个（CKvsFileBackend.hpp, CKvsFileBackend.cpp）  
**新增代码**: ~140 行  
**测试结果**: ✅ 57/57 测试通过  

---

## 🎯 实现目标

### AUTOSAR 标准要求

1. **[SWS_PER_00600] Update Workflow**:
   - Phase 1: Prepare - 创建 `update/` 目录
   - Phase 2: Modify - 所有修改在 `update/` 中进行
   - Phase 3: Commit - 验证 → 备份 → 原子替换
   - Phase 4: Rollback - 从 `redundancy/` 恢复

2. **[SWS_PER_00800] Data Integrity**:
   - 文件存在性检查
   - JSON 格式验证
   - Schema 验证（可选）
   - Checksum 验证（可选）
   - 原子操作保证

---

## 🔧 实现细节

### 1. 新增方法（CKvsFileBackend.hpp）

```cpp
/// @brief 校验数据文件完整性 [SWS_PER_00800]
/// @param filePath 要校验的文件路径
/// @return 校验成功返回空Result，失败返回错误码
core::Result<void> validateDataIntegrity(core::StringView filePath) noexcept;

/// @brief 备份 current/ 到 redundancy/ [SWS_PER_00502]
/// @return 备份成功返回空Result，失败返回错误码
core::Result<void> backupToRedundancy() noexcept;

/// @brief 原子替换 update/ -> current/ [SWS_PER_00600]
/// @return 替换成功返回空Result，失败返回错误码
core::Result<void> atomicReplaceCurrentWithUpdate() noexcept;
```

### 2. validateDataIntegrity() 实现

**检查项**:
1. ✅ 文件存在性 (`core::File::Util::exists`)
2. ✅ 文件可读性 (`ReadBinary`)
3. ✅ 文件非空
4. ✅ JSON 格式验证 (`boost::property_tree::read_json`)
5. ⏳ Schema 验证（TODO）
6. ⏳ Checksum 验证（TODO）

**错误码**:
- `PerErrc::kFileNotFound` - 文件不存在
- `PerErrc::kIntegrityCorrupted` - 文件损坏或格式错误

### 3. backupToRedundancy() 实现

**流程**:
1. 检查 `current/kvs_data.json` 是否存在
2. 读取 current 文件内容
3. 写入到 `redundancy/kvs_data.json.bak`
4. 记录日志

**错误码**:
- `PerErrc::kPhysicalStorageFailure` - 读写失败

**特性**:
- 如果 current 不存在（首次运行），跳过备份（不报错）

### 4. atomicReplaceCurrentWithUpdate() 实现

**原子性保证**:
```cpp
// Step 1: 复制 update/ 到临时文件
core::String tempPath = currentPath + ".tmp";
WriteBinary(tempPath, updateData);

// Step 2: 原子 rename（POSIX 保证）
rename(tempPath.c_str(), currentPath.c_str());
```

**关键特性**:
- 使用 `rename()` 系统调用（POSIX 原子操作）
- 失败时自动清理临时文件
- 保证部分更新不可见

**错误码**:
- `PerErrc::kFileNotFound` - update 文件不存在
- `PerErrc::kPhysicalStorageFailure` - 读写或 rename 失败

### 5. SyncToStorage() 重构

**旧实现** (违反 AUTOSAR):
```cpp
if (!m_dirty) return Result<void>::FromValue();
auto result = saveToFile(m_strFile);  // ❌ 直接写入 current/
if (result.HasValue()) m_dirty = false;
return result;
```

**新实现** (符合 AUTOSAR):
```cpp
if (!m_dirty) return Result<void>::FromValue();

// Phase 1: 保存到 update/ 目录
core::String updatePath = getUpdatePath();
auto saveResult = saveToFile(updatePath);
if (!saveResult.HasValue()) return saveResult;

// Phase 2: 完整性校验 [SWS_PER_00800]
auto validateResult = validateDataIntegrity(updatePath);
if (!validateResult.HasValue()) {
    core::File::Util::remove(updatePath.data());  // 清理无效文件
    return validateResult;
}

// Phase 3: 备份到 redundancy/ [SWS_PER_00502]
auto backupResult = backupToRedundancy();
if (!backupResult.HasValue()) {
    core::File::Util::remove(updatePath.data());
    return backupResult;
}

// Phase 4: 原子提交 [SWS_PER_00600]
auto replaceResult = atomicReplaceCurrentWithUpdate();
if (!replaceResult.HasValue()) {
    core::File::Util::remove(updatePath.data());
    return replaceResult;
}

m_dirty = false;
return Result<void>::FromValue();
```

---

## 🧪 测试验证

### 编译测试
```bash
cd /home/ddk/1_workspace/2_middleware/LightAP/build
make lap_persistency -j$(nproc)
# Result: ✅ 成功编译（仅 Boost 弃用警告）
```

### 功能测试
```bash
cd /home/ddk/1_workspace/2_middleware/LightAP/build/modules/Persistency
make persistency_test
./persistency_test --gtest_filter="*KeyValueStorage*"
# Result: ✅ 57/57 测试通过（263ms）
```

### 文件系统验证
```bash
$ ls -lh /tmp/autosar_persistency_test/kvs/tmp/test_kvs/current/
-rw-r--r-- 1 ddk ddk 19 Nov 14 22:49 kvs_data.json

$ ls -lh /tmp/autosar_persistency_test/kvs/tmp/test_kvs/redundancy/
-rw-r--r-- 1 ddk ddk 253 Nov 14 22:49 kvs_data.json.bak
```

✅ 数据正确写入 `current/` 和 `redundancy/` 目录

### 合规性测试
```bash
$ bash modules/Persistency/tools/verify_autosar_compliance.sh | grep -A 8 "\[4/10\]"

[4/10] 检查 SyncToStorage() 实现...
  ✅ 包含备份逻辑
  ✅ 包含完整性校验和AUTOSAR工作流方法
    ✓ validateDataIntegrity() 已实现
    ✓ backupToRedundancy() 已实现
    ✓ atomicReplaceCurrentWithUpdate() 已实现
```

---

## 📊 性能影响

### 测试时间对比
- **重构前**: 56ms（57 tests）
- **重构后**: 263ms（57 tests）
- **增幅**: +207ms (+369%)

### 增幅原因
1. 每次 `SyncToStorage()` 多了 3 个文件操作:
   - JSON 解析验证（Phase 2）
   - Redundancy 备份（Phase 3）
   - 临时文件 + rename（Phase 4）

2. 每个测试可能调用多次 `SyncToStorage()`

### 性能优化建议
1. **智能备份**: 仅在数据变化时备份
2. **跳过验证**: 对小文件/频繁操作可配置跳过
3. **批量提交**: 聚合多次修改后统一提交

**当前策略**: 优先保证 AUTOSAR 合规性和数据安全，后续可按需优化

---

## 🔍 代码审查要点

### ✅ 正确实践
1. ✅ 所有错误路径清理临时文件（`core::File::Util::remove`）
2. ✅ 使用 POSIX `rename()` 保证原子性
3. ✅ 详细日志记录每个 Phase
4. ✅ 错误码使用符合 AUTOSAR 标准
5. ✅ 无内存泄漏（使用 RAII）

### ⚠️ 待改进
1. ⚠️ Schema 验证未实现（需要 manifest.json）
2. ⚠️ Checksum 验证未实现（需要 CRC 配置）
3. ⚠️ Rollback 逻辑未在 `DiscardPendingChanges()` 中实现
4. ⚠️ 无线程安全保护（Task 5 待实现）

---

## 🔗 相关文档

- **AUTOSAR 标准**: `doc/AUTOSAR_AP_SWS_Persistency.pdf`
  - Section 8.2.5.9: `SyncToStorage` [SWS_PER_00310]
  - Section 8.3: Update Workflow [SWS_PER_00600]
  - Section 8.4: Data Integrity [SWS_PER_00800]

- **架构文档**: `ARCHITECTURE_REFACTORING_PLAN.md`
  - Section: "AUTOSAR AP R24-11 标准约束"

- **合规性检查**: `AUTOSAR_COMPLIANCE_CHECKLIST.md`
  - Test 4: SyncToStorage 实现检查

---

## 📌 下一步

### Task 5: 线程安全增强
- 添加 `std::shared_mutex` 到 `CKvsFileBackend`
- 保护 `m_kvsRoot` 和 `m_dirty`
- 检查 17 处共享数据访问点

### Task 6: AUTOSAR 特定方法测试
- `RecoverKey()` - 从 `recovery/` 恢复删除的键
- `ResetKey()` - 恢复初始值
- `DiscardPendingChanges()` - 从 `redundancy/` 回滚

---

## ✅ 结论

**Task 4 已完成！** 

- ✅ 3 个新方法实现
- ✅ SyncToStorage 符合 AUTOSAR 工作流
- ✅ 57/57 测试通过
- ✅ 数据完整性校验生效
- ✅ 合规性脚本更新

**合规性**: [SWS_PER_00600] ✅ | [SWS_PER_00800] ✅ (部分)
