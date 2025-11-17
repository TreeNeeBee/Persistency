# Property Backend 共享内存配置快速参考

## 快速开始

### 默认配置（1MB）
```cpp
#include "CKvsPropertyBackend.hpp"

// 使用默认1MB共享内存
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile);
```

### 自定义配置
```cpp
// 小型设备：512KB
constexpr size_t smallSize = 512ul << 10;
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, smallSize);

// 标准应用：4MB
constexpr size_t mediumSize = 4ul << 20;
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, mediumSize);

// 大数据：16MB
constexpr size_t largeSize = 16ul << 20;
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, largeSize);
```

## 大小计算公式

```cpp
// 基于预期数据量计算
const int expectedKeys = 5000;
const int avgKeyLength = 30;      // 平均key长度
const int avgValueSize = 100;     // 平均value大小
const float safetyMargin = 1.5f;  // 50%安全边距

size_t estimatedSize = static_cast<size_t>(
    expectedKeys * (avgKeyLength + avgValueSize) * safetyMargin
);

// 向上取整到MB
size_t sizeInMB = (estimatedSize / (1024 * 1024)) + 1;
size_t finalSize = sizeInMB << 20;

KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, finalSize);
```

## 推荐配置表

| 应用场景 | 数据规模 | 推荐大小 | 代码示例 |
|---------|---------|---------|---------|
| IoT设备配置 | < 100个参数 | 512KB | `512ul << 10` |
| 应用程序设置 | 100-500个参数 | 1MB | 默认 |
| 服务配置 | 500-2000个参数 | 4MB | `4ul << 20` |
| 运行时缓存 | 2000-10000项 | 16MB | `16ul << 20` |
| 大型数据集 | > 10000项 | 32-64MB | `32ul << 20` |

## 常见问题

### Q: 如何知道需要多大内存？
**A:** 观察日志或遇到 `boost::interprocess::bad_alloc` 时增加大小。

### Q: 默认1MB够用吗？
**A:** 对于大多数配置管理场景（几百个参数）完全够用。

### Q: 能否动态调整大小？
**A:** 不能。大小在构造时确定，需要重新创建实例。

### Q: 多个实例共享内存吗？
**A:** 每个backend实例有独立的共享内存空间。

### Q: 内存不足会怎样？
**A:** SetValue操作返回错误，可通过检查返回值处理。

## 错误处理

```cpp
constexpr size_t shmSize = 1ul << 20;  // 1MB
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, shmSize);

for (int i = 0; i < 10000; ++i) {
    auto result = backend.SetValue("key_" + std::to_string(i), Int32(i));
    
    if (!result.HasValue()) {
        // 内存不足或其他错误
        std::cerr << "Failed to store key " << i << std::endl;
        std::cerr << "Consider increasing shared memory size" << std::endl;
        break;
    }
}
```

## 性能参考

基于benchmark测试结果：

| 内存大小 | 可存储Int32键值对 | 可存储10KB字符串 |
|---------|------------------|------------------|
| 512KB | ~2,000 | ~30 |
| 1MB (默认) | ~4,000 | ~60 |
| 4MB | ~16,000 | ~250 |
| 16MB | ~65,000 | 1,000+ |

*注：实际容量取决于key长度和value类型*

## 监控和调试

```cpp
KvsPropertyBackend backend("my_app", KvsBackendType::kvsFile, 4ul << 20);

// 检查当前key数量
auto countResult = backend.GetKeyCount();
if (countResult.HasValue()) {
    std::cout << "Current keys: " << countResult.Value() << std::endl;
}

// 估算使用量（粗略）
// 假设平均每个key-value占用250字节
size_t estimatedUsage = countResult.Value() * 250;
size_t configuredSize = 4ul << 20;  // 4MB
float usagePercent = (estimatedUsage * 100.0f) / configuredSize;

std::cout << "Estimated usage: " << usagePercent << "%" << std::endl;
if (usagePercent > 80.0f) {
    std::cout << "WARNING: Consider increasing shared memory size" << std::endl;
}
```

## 日志输出

配置后会看到类似日志：
```
[INFO] KvsPropertyBackend initialized with SHM name: shm_kvs_12345_my_app, 
       identifier: my_app, size: 4096 KB
```

确认配置生效。

## 示例程序

运行完整示例：
```bash
cd build/modules/Persistency
./property_backend_config_example
```

查看4种配置场景和推荐指南。
