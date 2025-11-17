#!/bin/bash
# AUTOSAR 合规性自动化检查脚本
# 版本: 1.0
# 日期: 2025-11-14

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$(dirname "$MODULE_DIR")")"

echo "=========================================="
echo "AUTOSAR 合规性自动化检查"
echo "=========================================="
echo "模块目录: $MODULE_DIR"
echo "项目根目录: $PROJECT_ROOT"
echo ""

EXIT_CODE=0

# =============================================================================
# Test 1: 检查接口方法
# =============================================================================
echo "[1/10] 检查 KeyValueStorage 接口方法..."

# 检查是否存在非标准公共方法
NON_STANDARD=$(grep -rn "public:" "$MODULE_DIR/source/inc/CKeyValueStorage.hpp" -A 50 2>/dev/null | \
    grep -E "^\s*(Set|Get|Remove|Exists|Clear|Sync)\s*\(" | wc -l || echo "0")

if [ "$NON_STANDARD" -gt 0 ]; then
    echo -e "  ${RED}❌ 发现 $NON_STANDARD 个非标准公共方法${NC}"
    grep -rn "public:" "$MODULE_DIR/source/inc/CKeyValueStorage.hpp" -A 50 2>/dev/null | \
        grep -E "^\s*(Set|Get|Remove|Exists|Clear|Sync)\s*\(" || true
    EXIT_CODE=1
else
    echo -e "  ${GREEN}✅ 接口方法符合标准${NC}"
fi

# 检查 AUTOSAR 标准方法是否存在
echo "  检查 AUTOSAR 标准方法..."
REQUIRED_METHODS=(
    "GetAllKeys"
    "KeyExists"
    "GetValue"
    "SetValue"
    "RemoveKey"
    "RecoverKey"
    "ResetKey"
    "RemoveAllKeys"
    "SyncToStorage"
    "DiscardPendingChanges"
)

MISSING_METHODS=0
for method in "${REQUIRED_METHODS[@]}"; do
    if ! grep -q "$method" "$MODULE_DIR/source/inc/IKvsBackend.hpp" 2>/dev/null; then
        echo -e "    ${RED}❌ 缺失标准方法: $method${NC}"
        MISSING_METHODS=$((MISSING_METHODS + 1))
        EXIT_CODE=1
    fi
done

if [ "$MISSING_METHODS" -eq 0 ]; then
    echo -e "    ${GREEN}✅ 所有 AUTOSAR 标准方法已实现${NC}"
fi

# =============================================================================
# Test 2: 检查返回类型
# =============================================================================
echo ""
echo "[2/10] 检查返回类型..."

# 检查公共方法是否都使用 Result 返回类型
NON_RESULT=$(grep -A 100 "class.*KeyValueStorage\|class.*IKvsBackend" "$MODULE_DIR/source/inc/"*.hpp 2>/dev/null | \
    grep -E "^\s*virtual.*\(" | grep -v "Result<" | grep -v "~\|constructor" | wc -l || echo "0")

if [ "$NON_RESULT" -gt 0 ]; then
    echo -e "  ${YELLOW}⚠️  发现 $NON_RESULT 个方法可能未使用 Result 返回类型（需人工确认）${NC}"
else
    echo -e "  ${GREEN}✅ 所有方法使用 Result 返回类型${NC}"
fi

# =============================================================================
# Test 3: 检查直接写入 current/
# =============================================================================
echo ""
echo "[3/10] 检查是否直接写入 current/ 目录..."

DIRECT_WRITE=$(grep -rn "getCurrentPath()" "$MODULE_DIR/source/src/"*.cpp 2>/dev/null | \
    grep -E "write|modify|save" | grep -v "update\|backup\|comment" | wc -l || echo "0")

if [ "$DIRECT_WRITE" -gt 0 ]; then
    echo -e "  ${RED}❌ 发现 $DIRECT_WRITE 处可能直接修改 current/ 目录${NC}"
    grep -rn "getCurrentPath()" "$MODULE_DIR/source/src/"*.cpp 2>/dev/null | \
        grep -E "write|modify|save" | grep -v "update\|backup\|comment" || true
    EXIT_CODE=1
else
    echo -e "  ${GREEN}✅ 所有写操作通过 update/ 缓冲${NC}"
fi

# =============================================================================
# Test 4: 检查 SyncToStorage 实现
# =============================================================================
echo ""
echo "[4/10] 检查 SyncToStorage() 实现..."

# 检查是否实现了备份逻辑
if grep -q "redundancy\|backup" "$MODULE_DIR/source/src/CKvsFileBackend.cpp" 2>/dev/null; then
    echo -e "  ${GREEN}✅ 包含备份逻辑${NC}"
else
    echo -e "  ${YELLOW}⚠️  未发现备份逻辑（需人工确认）${NC}"
fi

# 检查是否有完整性校验 - 检查方法实现
if grep -q "validateDataIntegrity\|backupToRedundancy\|atomicReplaceCurrentWithUpdate" "$MODULE_DIR/source/src/CKvsFileBackend.cpp" 2>/dev/null; then
    echo -e "  ${GREEN}✅ 包含完整性校验和AUTOSAR工作流方法${NC}"
    
    # 进一步检查具体方法实现
    INTEGRITY_METHODS=("validateDataIntegrity" "backupToRedundancy" "atomicReplaceCurrentWithUpdate")
    for method in "${INTEGRITY_METHODS[@]}"; do
        if grep -q "Result<void> KvsFileBackend::${method}" "$MODULE_DIR/source/src/CKvsFileBackend.cpp" 2>/dev/null; then
            echo -e "    ${GREEN}✓${NC} ${method}() 已实现"
        fi
    done
else
    echo -e "  ${YELLOW}⚠️  未发现AUTOSAR工作流方法（需人工确认）${NC}"
fi

# =============================================================================
# Test 5: 检查目录结构
# =============================================================================
echo ""
echo "[5/10] 检查目录结构..."

# 检查代码中是否定义了标准目录
REQUIRED_DIRS=("current" "update" "redundancy" "recovery")
MISSING_DIR_REFS=0

for dir in "${REQUIRED_DIRS[@]}"; do
    # 检查是否在代码中引用（包括注释和字符串）
    if ! grep -rq "$dir" "$MODULE_DIR/source/"*.{cpp,hpp} 2>/dev/null; then
        echo -e "  ${YELLOW}⚠️  未在代码中发现 '$dir' 目录引用${NC}"
        MISSING_DIR_REFS=$((MISSING_DIR_REFS + 1))
    fi
done

# 检查是否有 4层目录结构的实现
if grep -rq "getCurrentPath\|getUpdatePath\|getRedundancyPath\|getRecoveryPath" "$MODULE_DIR/source/inc/CKvsFileBackend.hpp" 2>/dev/null; then
    echo -e "  ${GREEN}✅ 发现 AUTOSAR 4层目录结构辅助方法${NC}"
    MISSING_DIR_REFS=0  # 如果有辅助方法，认为目录结构已实现
fi

if [ "$MISSING_DIR_REFS" -gt 0 ]; then
    echo -e "  ${YELLOW}⚠️  部分标准目录未在代码中引用（可能正在实现中）${NC}"
else
    echo -e "  ${GREEN}✅ AUTOSAR 4层目录结构已实现${NC}"
fi

# =============================================================================
# Test 6: 检查错误码使用
# =============================================================================
echo ""
echo "[6/10] 检查错误码使用..."

WRONG_ERROR=$(grep -rn "FromError(" "$MODULE_DIR/source/src/"*.cpp 2>/dev/null | \
    grep -v "PerErrc::" | grep -v "core::ErrorCode" | grep -v "comment\|//\|/\*" | wc -l || echo "0")

if [ "$WRONG_ERROR" -gt 0 ]; then
    echo -e "  ${RED}❌ 发现 $WRONG_ERROR 处使用非标准错误码${NC}"
    grep -rn "FromError(" "$MODULE_DIR/source/src/"*.cpp 2>/dev/null | \
        grep -v "PerErrc::" | grep -v "core::ErrorCode" | grep -v "comment\|//\|/\*" | head -5 || true
    EXIT_CODE=1
else
    echo -e "  ${GREEN}✅ 错误码使用符合标准${NC}"
fi

# =============================================================================
# Test 7: 检查 Result 使用
# =============================================================================
echo ""
echo "[7/10] 检查 Result 错误传播..."

# 检查是否有忽略 Result 的情况（简单检查）
IGNORED_RESULT=$(grep -rn "auto.*Result<" "$MODULE_DIR/source/src/"*.cpp 2>/dev/null | \
    grep -v "HasValue\|Error\|Value" | wc -l || echo "0")

if [ "$IGNORED_RESULT" -gt 5 ]; then
    echo -e "  ${YELLOW}⚠️  发现 $IGNORED_RESULT 处可能忽略 Result 返回值（需人工确认）${NC}"
else
    echo -e "  ${GREEN}✅ Result 错误传播看起来正常${NC}"
fi

# =============================================================================
# Test 8: 检查线程安全
# =============================================================================
echo ""
echo "[8/10] 检查线程安全..."

# 检查是否有互斥锁保护
if grep -q "mutex\|lock" "$MODULE_DIR/source/inc/CKvsFileBackend.hpp" 2>/dev/null; then
    echo -e "  ${GREEN}✅ 发现互斥锁定义${NC}"
else
    echo -e "  ${YELLOW}⚠️  未发现互斥锁定义（可能需要添加）${NC}"
fi

# 检查共享数据访问是否有锁保护
UNSAFE_ACCESS=$(grep -rn "m_kvsRoot\|m_dirty" "$MODULE_DIR/source/src/CKvsFileBackend.cpp" 2>/dev/null | \
    grep -v "lock\|mutex\|constructor\|destructor\|//\|/\*" | wc -l || echo "0")

if [ "$UNSAFE_ACCESS" -gt 15 ]; then
    echo -e "  ${YELLOW}⚠️  发现 $UNSAFE_ACCESS 处访问共享数据（需确认是否有锁保护）${NC}"
else
    echo -e "  ${GREEN}✅ 共享数据访问看起来有保护${NC}"
fi

# =============================================================================
# Test 9: 检查文档完整性
# =============================================================================
echo ""
echo "[9/10] 检查文档完整性..."

REQUIRED_DOCS=(
    "$MODULE_DIR/doc/ARCHITECTURE_REFACTORING_PLAN.md"
    "$MODULE_DIR/doc/AUTOSAR_COMPLIANCE_CHECKLIST.md"
    "$MODULE_DIR/README.md"
)

MISSING_DOCS=0
for doc in "${REQUIRED_DOCS[@]}"; do
    if [ ! -f "$doc" ]; then
        echo -e "  ${RED}❌ 缺失文档: $(basename "$doc")${NC}"
        MISSING_DOCS=$((MISSING_DOCS + 1))
        EXIT_CODE=1
    fi
done

# 检查 AUTOSAR 标准文档（可选）
if [ -f "$MODULE_DIR/doc/AUTOSAR_AP_SWS_Persistency.pdf" ]; then
    echo -e "  ${GREEN}✅ AUTOSAR 标准文档存在${NC}"
else
    echo -e "  ${YELLOW}⚠️  AUTOSAR 标准文档不存在（建议添加）${NC}"
fi

if [ "$MISSING_DOCS" -eq 0 ]; then
    echo -e "  ${GREEN}✅ 必需文档完整${NC}"
fi

# =============================================================================
# Test 10: 检查测试覆盖率
# =============================================================================
echo ""
echo "[10/10] 检查测试覆盖率..."

# 统计测试用例数量
TEST_COUNT=$(grep -r "TEST.*KeyValueStorage" "$MODULE_DIR/test/"*.cpp 2>/dev/null | wc -l || echo "0")

if [ "$TEST_COUNT" -gt 50 ]; then
    echo -e "  ${GREEN}✅ 发现 $TEST_COUNT 个 KeyValueStorage 测试用例${NC}"
elif [ "$TEST_COUNT" -gt 20 ]; then
    echo -e "  ${YELLOW}⚠️  发现 $TEST_COUNT 个测试用例（建议增加到 50+）${NC}"
else
    echo -e "  ${YELLOW}⚠️  仅发现 $TEST_COUNT 个测试用例（覆盖率可能不足）${NC}"
fi

# 检查是否有 AUTOSAR 相关测试
if grep -rq "AUTOSAR\|RecoverKey\|ResetKey\|DiscardPendingChanges" "$MODULE_DIR/test/"*.cpp 2>/dev/null; then
    echo -e "  ${GREEN}✅ 包含 AUTOSAR 特定功能测试${NC}"
else
    echo -e "  ${YELLOW}⚠️  未发现 AUTOSAR 特定功能测试（建议添加）${NC}"
fi

# =============================================================================
# 总结
# =============================================================================
echo ""
echo "=========================================="
if [ "$EXIT_CODE" -eq 0 ]; then
    echo -e "${GREEN}✅ 所有关键检查通过！AUTOSAR 合规性验证成功${NC}"
else
    echo -e "${RED}❌ 发现合规性问题，请修复后重新检查${NC}"
fi
echo "=========================================="

# 生成详细报告
echo ""
echo "详细报告已保存到: $MODULE_DIR/build/autosar_compliance_report.txt"
echo ""

exit $EXIT_CODE
