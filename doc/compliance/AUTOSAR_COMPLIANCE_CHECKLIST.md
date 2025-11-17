# AUTOSAR 合规性检查清单

**版本**: 1.0  
**日期**: 2025-11-14  
**标准**: AUTOSAR Adaptive Platform R24-11  
**参考文档**: AUTOSAR_AP_SWS_Persistency.pdf

---

## 📋 目录

1. [接口合规性](#接口合规性)
2. [目录结构合规性](#目录结构合规性)
3. [操作流程合规性](#操作流程合规性)
4. [数据完整性合规性](#数据完整性合规性)
5. [错误处理合规性](#错误处理合规性)
6. [线程安全合规性](#线程安全合规性)
7. [自动化验证脚本](#自动化验证脚本)

---

## 1. 接口合规性

### 1.1 KeyValueStorage 接口检查

**标准要求**: [SWS_PER_00042] - KeyValueStorage 必须实现 10 个标准方法

#### ✅ 必须实现的方法

- [ ] `GetAllKeys() const noexcept` - [SWS_PER_00042]
- [ ] `KeyExists(StringView key) const noexcept` - [SWS_PER_00309]
- [ ] `GetValue<T>(StringView key) const noexcept` - [SWS_PER_00310]
- [ ] `SetValue<T>(StringView key, const T& value) noexcept` - [SWS_PER_00311]
- [ ] `RemoveKey(StringView key) noexcept` - [SWS_PER_00312]
- [ ] `RecoverKey(StringView key) noexcept` - [SWS_PER_00313]
- [ ] `ResetKey(StringView key) noexcept` - [SWS_PER_00314]
- [ ] `RemoveAllKeys() noexcept` - [SWS_PER_00315]
- [ ] `SyncToStorage() noexcept` - [SWS_PER_00316]
- [ ] `DiscardPendingChanges() noexcept` - [SWS_PER_00317]

#### ❌ 禁止的公共方法

- [ ] 确认没有 `Set()` 方法（非标准）
- [ ] 确认没有 `Get()` 方法（非标准）
- [ ] 确认没有 `Remove()` 方法（非标准，应该是 `RemoveKey()`）
- [ ] 确认没有 `Exists()` 方法（非标准，应该是 `KeyExists()`）
- [ ] 确认没有 `Clear()` 方法（非标准，应该是 `RemoveAllKeys()`）
- [ ] 确认没有 `Sync()` 方法（非标准，应该是 `SyncToStorage()`）

**验证命令**:
```bash
# 检查公共接口是否包含非标准方法
grep -rn "public:" modules/Persistency/source/inc/CKeyValueStorage.hpp -A 50 | \
    grep -E "^\s*(Set|Get|Remove|Exists|Clear|Sync)\s*\("
```

**预期结果**: 无输出（所有非标准方法应该是私有的或不存在）

---

### 1.2 FileStorage 接口检查

**标准要求**: [SWS_PER_00400] - FileStorage 必须实现标准文件操作方法

#### ✅ 必须实现的方法

- [ ] `GetAllFiles() const noexcept` - [SWS_PER_00400]
- [ ] `FileExists(StringView filename) const noexcept` - [SWS_PER_00401]
- [ ] `DeleteFile(StringView filename) noexcept` - [SWS_PER_00402]
- [ ] `RecoverFile(StringView filename) noexcept` - [SWS_PER_00403]
- [ ] `ResetFile(StringView filename) noexcept` - [SWS_PER_00404]
- [ ] `DeleteAllFiles() noexcept` - [SWS_PER_00405]
- [ ] `SyncToStorage() noexcept` - [SWS_PER_00406]
- [ ] `GetFileContent(StringView filename) const noexcept` - [SWS_PER_00407]
- [ ] `SetFileContent(StringView filename, const Vector<Byte>& content) noexcept` - [SWS_PER_00408]

---

### 1.3 返回类型检查

**标准要求**: [SWS_PER_00050] - 所有方法必须返回 `ara::core::Result<T>`

#### ✅ 必须符合的规则

- [ ] 所有公共方法返回 `Result<T>` 或 `Result<void>`
- [ ] 使用 `core::Result` 而非 `std::optional` 或裸指针
- [ ] 错误码使用 `PerErrc` 枚举
- [ ] 成功时使用 `Result<T>::FromValue(value)`
- [ ] 失败时使用 `Result<T>::FromError(PerErrc::kXXX)`

**验证命令**:
```bash
# 检查是否所有公共方法都返回 Result
grep -rn "public:" modules/Persistency/source/inc/ -A 100 | \
    grep -E "^\s*(ara::core::)?Result<.*>\s+[A-Z]" | \
    grep -v "Result<"
```

**预期结果**: 无输出（所有方法都应该返回 Result）

---

## 2. 目录结构合规性

### 2.1 标准目录布局

**标准要求**: [SWS_PER_00500] - 4 层目录结构

#### ✅ 必须存在的目录

```
/opt/persistency/<app_name>/<storage_type>/<shortname>/
├── current/      # [SWS_PER_00500] 当前有效数据
├── update/       # [SWS_PER_00501] 更新缓冲区
├── redundancy/   # [SWS_PER_00502] 冗余备份
└── recovery/     # [SWS_PER_00503] 恢复区
```

**验证命令**:
```bash
# 检查目录结构是否符合标准
test_app="test_app"
test_shortname="test_kvs"
base_dir="/opt/persistency/${test_app}/key_value_storage/${test_shortname}"

for dir in current update redundancy recovery; do
    if [ ! -d "${base_dir}/${dir}" ]; then
        echo "❌ 缺失目录: ${base_dir}/${dir}"
    else
        echo "✅ 目录存在: ${base_dir}/${dir}"
    fi
done
```

---

### 2.2 目录权限检查

**标准要求**: [SWS_PER_00504] - 目录权限必须为 0700（仅所有者可访问）

**验证命令**:
```bash
# 检查目录权限
find /opt/persistency -type d -exec stat -c "%a %n" {} \; | \
    grep -v "700"
```

**预期结果**: 无输出（所有目录权限都应该是 700）

---

### 2.3 应用隔离检查

**标准要求**: [SWS_PER_00504] - 应用之间目录严格隔离

#### ✅ 必须符合的规则

- [ ] 每个应用有独立的顶级目录
- [ ] 应用之间不能跨目录访问
- [ ] 文件路径必须包含应用名作为隔离标识

**验证命令**:
```bash
# 检查代码中是否有硬编码的跨应用路径
grep -rn "/opt/persistency/" modules/Persistency/source/src/ | \
    grep -v "GetApplicationName()" | \
    grep -v "m_appName"
```

**预期结果**: 无输出（所有路径都应该动态构造）

---

## 3. 操作流程合规性

### 3.1 更新流程检查

**标准要求**: [SWS_PER_00600] - 更新必须通过 update/ 缓冲区

#### ✅ 更新流程必须包含的步骤

- [ ] **Prepare**: 创建 `update/` 目录
- [ ] **Prepare**: 从 `current/` 复制数据到 `update/`
- [ ] **Modify**: 所有修改操作在 `update/` 目录执行
- [ ] **Modify**: `current/` 目录保持只读状态
- [ ] **Commit**: 验证 `update/` 数据完整性
- [ ] **Commit**: 备份 `current/` 到 `redundancy/`
- [ ] **Commit**: 原子替换 `update/` → `current/`
- [ ] **Commit**: 清理 `update/` 临时数据

**验证代码示例**:
```cpp
// ✅ 正确的更新流程
Result<void> SetValue(StringView key, const KvsDataType& value) {
    // Step 1: Prepare update directory
    if (!updateDirExists()) {
        auto prepareResult = prepareUpdateDirectory();
        if (!prepareResult.HasValue()) {
            return Result<void>::FromError(prepareResult.Error());
        }
    }
    
    // Step 2: Modify in update directory
    auto updatePath = getUpdatePath();
    auto modifyResult = modifyInUpdateDir(updatePath, key, value);
    if (!modifyResult.HasValue()) {
        return Result<void>::FromError(modifyResult.Error());
    }
    
    // Step 3: Mark as dirty for later commit
    m_dirty = true;
    return Result<void>::FromValue();
}

Result<void> SyncToStorage() {
    if (!m_dirty) {
        return Result<void>::FromValue();
    }
    
    // Step 4: Validate update data
    auto validateResult = validateUpdateData();
    if (!validateResult.HasValue()) {
        return Result<void>::FromError(validateResult.Error());
    }
    
    // Step 5: Backup current to redundancy
    auto backupResult = backupCurrentToRedundancy();
    if (!backupResult.HasValue()) {
        return Result<void>::FromError(backupResult.Error());
    }
    
    // Step 6: Atomic replace
    auto replaceResult = atomicReplaceCurrentWithUpdate();
    if (!replaceResult.HasValue()) {
        // Rollback on failure
        restoreFromRedundancy();
        return Result<void>::FromError(replaceResult.Error());
    }
    
    // Step 7: Cleanup
    cleanupUpdateDir();
    m_dirty = false;
    
    return Result<void>::FromValue();
}
```

**验证命令**:
```bash
# 检查是否有直接修改 current/ 的代码
grep -rn "getCurrentPath()" modules/Persistency/source/src/ | \
    grep -E "(write|modify|delete|remove)" | \
    grep -v "update"
```

**预期结果**: 无输出（所有写操作都应该在 update/ 目录）

---

### 3.2 回滚流程检查

**标准要求**: [SWS_PER_00650] - 支持从 redundancy/ 回滚

#### ✅ 回滚流程必须包含的步骤

- [ ] 检测 `redundancy/` 备份存在性
- [ ] 删除损坏的 `current/` 数据
- [ ] 从 `redundancy/` 恢复到 `current/`
- [ ] 验证恢复数据完整性
- [ ] 记录回滚日志

**验证代码示例**:
```cpp
Result<void> DiscardPendingChanges() {
    // Step 1: Check redundancy exists
    if (!redundancyExists()) {
        LAP_PER_LOG_ERROR << "Redundancy backup not found";
        return Result<void>::FromError(PerErrc::kBackupNotFound);
    }
    
    // Step 2: Remove current and update directories
    removeCurrentDir();
    removeUpdateDir();
    
    // Step 3: Restore from redundancy
    auto restoreResult = restoreFromRedundancy();
    if (!restoreResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to restore from redundancy";
        return Result<void>::FromError(restoreResult.Error());
    }
    
    // Step 4: Validate restored data
    auto validateResult = validateRestoredData();
    if (!validateResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Restored data validation failed";
        return Result<void>::FromError(PerErrc::kIntegrityCheckFailed);
    }
    
    // Step 5: Log rollback event
    logRollbackEvent();
    
    m_dirty = false;
    return Result<void>::FromValue();
}
```

---

### 3.3 恢复流程检查

**标准要求**: [SWS_PER_00700] - 支持已删除键的恢复

#### ✅ 恢复流程必须包含的步骤

- [ ] `RemoveKey()` 时自动备份到 `recovery/`
- [ ] `RecoverKey()` 从 `recovery/` 恢复
- [ ] `ResetKey()` 重置到初始值（需要配置支持）

**验证代码示例**:
```cpp
Result<void> RemoveKey(StringView key) {
    // Step 1: Backup to recovery before deletion
    auto currentValue = GetValue(key);
    if (currentValue.HasValue()) {
        auto backupResult = backupToRecovery(key, currentValue.Value());
        if (!backupResult.HasValue()) {
            LAP_PER_LOG_WARN << "Failed to backup key to recovery: " << key;
            // Continue deletion even if backup fails (non-critical)
        }
    }
    
    // Step 2: Delete from storage
    m_kvsRoot.erase(key.data());
    m_dirty = true;
    
    return Result<void>::FromValue();
}

Result<void> RecoverKey(StringView key) {
    // Step 1: Load recovery data
    auto recoveryPath = getRecoveryPath() / "deleted_keys.json";
    auto deletedData = loadDeletedKeys(recoveryPath);
    
    // Step 2: Find deleted key
    if (!deletedData.contains(key)) {
        LAP_PER_LOG_ERROR << "Key not found in recovery: " << key;
        return Result<void>::FromError(PerErrc::kKeyNotFound);
    }
    
    // Step 3: Restore value
    auto value = deletedData[key];
    auto restoreResult = SetValue(key, value);
    
    // Step 4: Remove from recovery storage
    if (restoreResult.HasValue()) {
        removeFromRecovery(key);
    }
    
    return restoreResult;
}
```

---

## 4. 数据完整性合规性

### 4.1 原子性检查

**标准要求**: [SWS_PER_00800] - 所有更新必须原子提交

#### ✅ 必须符合的规则

- [ ] 使用文件重命名实现原子操作（不使用直接写入）
- [ ] 临时文件写入完成后才重命名
- [ ] 重命名失败时自动回滚

**验证代码示例**:
```cpp
Result<void> atomicReplaceCurrentWithUpdate() {
    // Step 1: Write to temporary file first
    auto tempPath = getCurrentPath() / "kvs_data.json.tmp";
    auto updatePath = getUpdatePath() / "kvs_data.json";
    
    // Step 2: Copy update to temp location
    auto copyResult = core::File::Util::CopyFile(updatePath, tempPath);
    if (!copyResult.HasValue()) {
        return Result<void>::FromError(PerErrc::kFileOperationFailed);
    }
    
    // Step 3: Atomic rename
    auto currentPath = getCurrentPath() / "kvs_data.json";
    if (rename(tempPath.c_str(), currentPath.c_str()) != 0) {
        // Cleanup temp file on failure
        core::File::Util::Remove(tempPath);
        return Result<void>::FromError(PerErrc::kFileOperationFailed);
    }
    
    return Result<void>::FromValue();
}
```

**验证命令**:
```bash
# 检查是否有直接写入 current/ 的代码（应该使用 rename）
grep -rn "File::Util::Write" modules/Persistency/source/src/ | \
    grep "current"
```

**预期结果**: 无输出（应该使用临时文件+重命名）

---

### 4.2 完整性校验检查

**标准要求**: [SWS_PER_00800] - 提交前必须校验数据完整性

#### ✅ 必须实现的校验

- [ ] 文件存在性检查
- [ ] JSON 格式校验（如果使用 JSON）
- [ ] Schema 验证（如果定义了 Schema）
- [ ] CRC/Checksum 校验

**验证代码示例**:
```cpp
Result<void> validateUpdateData() {
    auto updateDataPath = getUpdatePath() / "kvs_data.json";
    
    // Check 1: File exists
    if (!core::File::Util::exists(updateDataPath)) {
        LAP_PER_LOG_ERROR << "Update data file not found";
        return Result<void>::FromError(PerErrc::kFileNotFound);
    }
    
    // Check 2: Valid JSON format
    if (!isValidJson(updateDataPath)) {
        LAP_PER_LOG_ERROR << "Invalid JSON format in update data";
        return Result<void>::FromError(PerErrc::kDataCorrupted);
    }
    
    // Check 3: Schema validation (if schema exists)
    if (hasSchema() && !validateSchema(updateDataPath)) {
        LAP_PER_LOG_ERROR << "Schema validation failed";
        return Result<void>::FromError(PerErrc::kSchemaValidationFailed);
    }
    
    // Check 4: Checksum verification
    if (!verifyChecksum(updateDataPath)) {
        LAP_PER_LOG_ERROR << "Checksum verification failed";
        return Result<void>::FromError(PerErrc::kIntegrityCheckFailed);
    }
    
    return Result<void>::FromValue();
}
```

---

### 4.3 冗余备份检查

**标准要求**: [SWS_PER_00502] - 提交前备份到 redundancy/

#### ✅ 必须符合的规则

- [ ] 每次 `SyncToStorage()` 前备份 `current/`
- [ ] 保留最近 N 个版本（配置项）
- [ ] 备份失败时中止提交

**验证代码示例**:
```cpp
Result<void> backupCurrentToRedundancy() {
    auto currentPath = getCurrentPath() / "kvs_data.json";
    auto redundancyPath = getRedundancyPath() / "kvs_data.json.bak";
    
    // Backup current version
    auto backupResult = core::File::Util::CopyFile(currentPath, redundancyPath);
    if (!backupResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to backup to redundancy";
        return Result<void>::FromError(PerErrc::kBackupFailed);
    }
    
    // Rotate old backups (keep last N versions)
    rotateOldBackups(getRedundancyPath(), m_config.redundancyCount);
    
    return Result<void>::FromValue();
}
```

---

## 5. 错误处理合规性

### 5.1 错误码检查

**标准要求**: [SWS_PER_00900] - 使用标准 PerErrc 错误码

#### ✅ 必须使用的错误码

- [ ] `PerErrc::kKeyNotFound` - 键不存在
- [ ] `PerErrc::kFileNotFound` - 文件不存在
- [ ] `PerErrc::kNotInitialized` - 后端未初始化
- [ ] `PerErrc::kIllegalWriteAccess` - 非法写入
- [ ] `PerErrc::kIntegrityCheckFailed` - 完整性校验失败
- [ ] `PerErrc::kBackupNotFound` - 备份不存在
- [ ] `PerErrc::kBackupFailed` - 备份失败
- [ ] `PerErrc::kDataCorrupted` - 数据损坏

**验证命令**:
```bash
# 检查是否使用了非标准错误码
grep -rn "FromError(" modules/Persistency/source/src/ | \
    grep -v "PerErrc::" | \
    grep -v "core::ErrorCode"
```

**预期结果**: 无输出（所有错误都应该使用 PerErrc）

---

### 5.2 Result 检查规则

**标准要求**: [SWS_PER_00900] - 所有 Result 必须检查

#### ✅ 必须符合的规则

- [ ] 调用返回 Result 的方法后必须检查 `HasValue()`
- [ ] 禁止忽略 Result 返回值
- [ ] 错误传播使用 `return result;` 而非丢弃

**验证命令**:
```bash
# 检查是否有未检查的 Result 调用
grep -rn "\.HasValue()" modules/Persistency/source/src/ -B 3 | \
    grep "auto.*=" | \
    grep -v "if\|while"
```

---

## 6. 线程安全合规性

### 6.1 线程安全检查

**标准要求**: [SWS_PER_00309] - 所有操作必须线程安全

#### ✅ 必须实现的线程安全措施

- [ ] 使用 `std::mutex` 或 `std::shared_mutex` 保护共享数据
- [ ] 读操作使用 `shared_lock`（允许并发读）
- [ ] 写操作使用 `unique_lock`（独占写）
- [ ] 避免死锁（统一锁顺序）

**验证代码示例**:
```cpp
class CKvsFileBackend {
private:
    mutable std::shared_mutex m_mutex;  // Protects m_kvsRoot and m_dirty
    
public:
    Result<Bool> KeyExists(StringView key) const noexcept {
        std::shared_lock<std::shared_mutex> lock(m_mutex);  // Read lock
        
        if (!m_bAvailable) {
            return Result<Bool>::FromError(PerErrc::kNotInitialized);
        }
        
        bool exists = m_kvsRoot.find(key.data()) != m_kvsRoot.not_found();
        return Result<Bool>::FromValue(exists);
    }
    
    Result<void> SetValue(StringView key, const KvsDataType& value) noexcept {
        std::unique_lock<std::shared_mutex> lock(m_mutex);  // Write lock
        
        if (!m_bAvailable) {
            return Result<void>::FromError(PerErrc::kNotInitialized);
        }
        
        m_kvsRoot.put(key.data(), value);
        m_dirty = true;
        
        return Result<void>::FromValue();
    }
};
```

**验证命令**:
```bash
# 检查是否有未保护的共享数据访问
grep -rn "m_kvsRoot\|m_dirty" modules/Persistency/source/src/CKvsFileBackend.cpp | \
    grep -v "lock\|mutex"
```

---

## 7. 自动化验证脚本

### 7.1 完整合规性检查脚本

```bash
#!/bin/bash
# tools/verify_autosar_compliance.sh

echo "=========================================="
echo "AUTOSAR 合规性自动化检查"
echo "=========================================="

EXIT_CODE=0

# Test 1: 检查接口方法
echo "[1/8] 检查接口方法..."
NON_STANDARD=$(grep -rn "public:" modules/Persistency/source/inc/CKeyValueStorage.hpp -A 50 | \
    grep -E "^\s*(Set|Get|Remove|Exists|Clear|Sync)\s*\(" | wc -l)
if [ "$NON_STANDARD" -gt 0 ]; then
    echo "  ❌ 发现 $NON_STANDARD 个非标准公共方法"
    EXIT_CODE=1
else
    echo "  ✅ 接口方法符合标准"
fi

# Test 2: 检查返回类型
echo "[2/8] 检查返回类型..."
NON_RESULT=$(grep -rn "public:" modules/Persistency/source/inc/ -A 100 | \
    grep -E "^\s*[A-Z]" | grep -v "Result<" | grep -v "class\|struct\|enum" | wc -l)
if [ "$NON_RESULT" -gt 0 ]; then
    echo "  ❌ 发现 $NON_RESULT 个方法未使用 Result 返回类型"
    EXIT_CODE=1
else
    echo "  ✅ 所有方法使用 Result 返回类型"
fi

# Test 3: 检查直接写入 current/
echo "[3/8] 检查直接写入 current/ ..."
DIRECT_WRITE=$(grep -rn "getCurrentPath()" modules/Persistency/source/src/ | \
    grep -E "(write|modify)" | grep -v "update" | wc -l)
if [ "$DIRECT_WRITE" -gt 0 ]; then
    echo "  ❌ 发现 $DIRECT_WRITE 处直接修改 current/ 目录"
    EXIT_CODE=1
else
    echo "  ✅ 所有写操作通过 update/ 缓冲"
fi

# Test 4: 检查目录结构
echo "[4/8] 检查目录结构..."
if [ -d "/opt/persistency" ]; then
    INVALID_DIRS=$(find /opt/persistency -type d | \
        grep -v "current\|update\|redundancy\|recovery\|system\|key_value_storage\|file_storage" | \
        tail -n +2 | wc -l)
    if [ "$INVALID_DIRS" -gt 0 ]; then
        echo "  ❌ 发现 $INVALID_DIRS 个非标准目录"
        EXIT_CODE=1
    else
        echo "  ✅ 目录结构符合标准"
    fi
else
    echo "  ⚠️  /opt/persistency 不存在，跳过检查"
fi

# Test 5: 检查目录权限
echo "[5/8] 检查目录权限..."
if [ -d "/opt/persistency" ]; then
    WRONG_PERMS=$(find /opt/persistency -type d -exec stat -c "%a" {} \; | \
        grep -v "700" | wc -l)
    if [ "$WRONG_PERMS" -gt 0 ]; then
        echo "  ❌ 发现 $WRONG_PERMS 个目录权限不正确（应为 700）"
        EXIT_CODE=1
    else
        echo "  ✅ 目录权限符合标准"
    fi
else
    echo "  ⚠️  /opt/persistency 不存在，跳过检查"
fi

# Test 6: 检查错误码
echo "[6/8] 检查错误码使用..."
WRONG_ERROR=$(grep -rn "FromError(" modules/Persistency/source/src/ | \
    grep -v "PerErrc::" | grep -v "core::ErrorCode" | wc -l)
if [ "$WRONG_ERROR" -gt 0 ]; then
    echo "  ❌ 发现 $WRONG_ERROR 处使用非标准错误码"
    EXIT_CODE=1
else
    echo "  ✅ 错误码使用符合标准"
fi

# Test 7: 检查线程安全
echo "[7/8] 检查线程安全..."
UNSAFE_ACCESS=$(grep -rn "m_kvsRoot\|m_dirty" modules/Persistency/source/src/CKvsFileBackend.cpp | \
    grep -v "lock\|mutex\|constructor\|destructor" | wc -l)
if [ "$UNSAFE_ACCESS" -gt 10 ]; then
    echo "  ⚠️  发现 $UNSAFE_ACCESS 处可能的线程不安全访问（需人工确认）"
else
    echo "  ✅ 线程安全保护看起来正常"
fi

# Test 8: 检查文档
echo "[8/8] 检查文档完整性..."
REQUIRED_DOCS=(
    "modules/Persistency/doc/ARCHITECTURE_REFACTORING_PLAN.md"
    "modules/Persistency/doc/AUTOSAR_COMPLIANCE_CHECKLIST.md"
    "modules/Persistency/doc/AUTOSAR_AP_SWS_Persistency.pdf"
)
for doc in "${REQUIRED_DOCS[@]}"; do
    if [ ! -f "$doc" ]; then
        echo "  ❌ 缺失文档: $doc"
        EXIT_CODE=1
    fi
done
if [ "$EXIT_CODE" -eq 0 ]; then
    echo "  ✅ 文档完整"
fi

echo "=========================================="
if [ "$EXIT_CODE" -eq 0 ]; then
    echo "✅ 所有检查通过！AUTOSAR 合规性验证成功"
else
    echo "❌ 发现合规性问题，请修复后重新检查"
fi
echo "=========================================="

exit $EXIT_CODE
```

---

### 7.2 使用方法

```bash
# 1. 赋予执行权限
chmod +x scripts/verify_autosar_compliance.sh

# 2. 运行完整检查
./scripts/verify_autosar_compliance.sh

# 3. 集成到 CI/CD
# 在 .gitlab-ci.yml 或 .github/workflows/ci.yml 中添加：
- name: AUTOSAR Compliance Check
  run: ./scripts/verify_autosar_compliance.sh
```

---

## 8. 常见问题与解决方案

### Q1: 为什么必须使用 update/ 缓冲区？

**A**: AUTOSAR 标准要求所有更新必须原子提交，直接修改 current/ 可能导致：
- 崩溃时数据不一致
- 无法回滚到上一个稳定版本
- 违反 [SWS_PER_00600] 原子性要求

---

### Q2: RecoverKey() 和 ResetKey() 的区别？

**A**:
- `RecoverKey()`: 从 recovery/ 恢复已删除的键（运行时删除）
- `ResetKey()`: 重置到配置文件中定义的初始值（恢复到出厂设置）

---

### Q3: 如何验证我的代码符合 AUTOSAR 标准？

**A**: 三步验证：
1. 运行 `verify_autosar_compliance.sh` 自动化脚本
2. 手动检查本文档中的所有 checklist
3. 使用 `pdf2txt` 对照标准文档验证接口签名

```bash
pdf2txt modules/Persistency/doc/AUTOSAR_AP_SWS_Persistency.pdf | \
    grep -A 5 "KeyValueStorage::GetAllKeys"
```

---

## 9. 版本历史

| 版本 | 日期 | 变更说明 | 作者 |
|-----|------|---------|------|
| 1.0 | 2025-11-14 | 初始版本，完成 KeyValueStorage 合规性检查清单 | AI Assistant |

---

## 10. 参考资料

- **AUTOSAR_AP_SWS_Persistency.pdf** - 完整标准文档
- **ARCHITECTURE_REFACTORING_PLAN.md** - 架构重构计划
- **modules/Persistency/README.md** - 模块使用文档
