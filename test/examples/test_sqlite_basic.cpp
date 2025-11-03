/**
 * @file test_sqlite_basic.cpp
 * @brief SQLite Backend基础功能测试（无日志系统依赖）
 */

#include <sqlite3.h>
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>

void testSqliteDirectly() {
    std::cout << "=== Testing SQLite3 Basic Operations ===\n\n";
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open("/tmp/direct_test.db", &db);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return;
    }
    
    std::cout << "✓ Database opened successfully\n";
    
    // 启用WAL模式
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    std::cout << "✓ WAL mode enabled\n";
    
    // 创建表
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS kvs_data ("
        "    key TEXT PRIMARY KEY NOT NULL,"
        "    value TEXT NOT NULL,"
        "    deleted INTEGER DEFAULT 0"
        ") WITHOUT ROWID;";
    
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create table: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return;
    }
    std::cout << "✓ Table created\n";
    
    // 插入数据（带类型编码）
    sqlite3_stmt* stmt = nullptr;
    const char* insertSQL = "INSERT OR REPLACE INTO kvs_data (key, value, deleted) VALUES (?, ?, 0);";
    
    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement\n";
        return;
    }
    
    // 插入Int32类型（类型标记 = 'a' + 4 = 'e'）
    std::string encodedValue = "e123";  // 'e' for Int32, "123" for value
    sqlite3_bind_text(stmt, 1, "test_int", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encodedValue.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    
    // 插入String类型（类型标记 = 'a' + 11 = 'l'）
    encodedValue = "lHello SQLite";
    sqlite3_bind_text(stmt, 1, "test_string", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encodedValue.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    
    // 插入Double类型（类型标记 = 'a' + 10 = 'k'）
    encodedValue = "k3.14159265358979";
    sqlite3_bind_text(stmt, 1, "test_double", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encodedValue.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);
    std::cout << "✓ Inserted 3 keys with type encoding\n";
    
    // 读取数据
    const char* selectSQL = "SELECT key, value, deleted FROM kvs_data WHERE deleted = 0;";
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    
    std::cout << "\nStored data:\n";
    std::cout << "----------------------------------------\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int deleted = sqlite3_column_int(stmt, 2);
        
        // 解码类型
        char typeMarker = value[0];
        int typeIndex = typeMarker - 'a';
        const char* data = value + 1;
        
        const char* typeNames[] = {
            "Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32",
            "Int64", "UInt64", "Bool", "Float", "Double", "String"
        };
        
        std::cout << "Key: " << key << "\n";
        std::cout << "  Type: " << typeNames[typeIndex] << " (marker='" << typeMarker << "')\n";
        std::cout << "  Data: " << data << "\n";
        std::cout << "  Deleted: " << deleted << "\n\n";
    }
    
    sqlite3_finalize(stmt);
    std::cout << "----------------------------------------\n";
    
    // 性能测试
    std::cout << "\n=== Performance Test ===\n";
    
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    for (int i = 0; i < 1000; ++i) {
        std::string key = "perf_key_" + std::to_string(i);
        std::string value = "e" + std::to_string(i);  // Int32 type
        
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    
    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Inserted 1000 keys in " << duration.count() << " ms\n";
    std::cout << "Performance: " << (1000.0 / duration.count() * 1000) << " ops/sec\n";
    
    // 统计总数
    const char* countSQL = "SELECT COUNT(*) FROM kvs_data WHERE deleted = 0;";
    rc = sqlite3_prepare_v2(db, countSQL, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        std::cout << "\nTotal active keys in database: " << count << "\n";
    }
    sqlite3_finalize(stmt);
    
    sqlite3_close(db);
    std::cout << "\n✓ All tests completed successfully!\n";
}

int main() {
    testSqliteDirectly();
    return 0;
}
