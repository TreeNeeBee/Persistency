#ifndef LAP_PERSISTENCY_DAEMON_CPERSISTENCYDAEMON_HPP_
#define LAP_PERSISTENCY_DAEMON_CPERSISTENCYDAEMON_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <boost/thread/shared_mutex.hpp>

#include <iostream>

#include "CPersistency.hpp"

namespace lap {
namespace pm {
namespace daemon {

/**
 * @brief Android property style daemon using domain socket
 * 
 * Key naming convention:
 * - persist.* : persistent keys that are written to SQLite immediately
 * - ro.* : read-only keys (set once at startup)
 * - sys.* : system keys (volatile, in-memory only)
 * - vendor.* : vendor specific keys
 */
class PersistencyDaemon {
public:
    static constexpr const char* SOCKET_PATH = "/dev/socket/property_service";
    static constexpr size_t MAX_PROPERTY_KEY_LEN = 32;
    static constexpr size_t MAX_PROPERTY_VALUE_LEN = 92;
    static constexpr size_t MAX_MESSAGE_SIZE = 1024;
    
    struct PropertyEntry {
        std::string value;
        EKvsDataTypeIndicate type;
        bool persistent;
        uint64_t timestamp;
        
        PropertyEntry() : type(EKvsDataTypeIndicate::DataType_string), persistent(false), timestamp(0) {}
        PropertyEntry(const std::string& v, EKvsDataTypeIndicate t, bool p = false) 
            : value(v), type(t), persistent(p), timestamp(getCurrentTimestamp()) {}
            
    private:
        static uint64_t getCurrentTimestamp();
    };

public:
    PersistencyDaemon();
    PersistencyDaemon(const std::string& socketPath, const std::string& dbPath);
    ~PersistencyDaemon();

    // Main daemon lifecycle
    core::Result<void> initialize();
    core::Result<void> start();
    void stop();
    bool isRunning() const { return m_running.load(); }

private:
    // Socket management
    core::Result<void> createSocket();
    void handleConnections();
    void handleClient(int clientFd);
    
    // Message processing
    core::Result<void> processMessage(const std::vector<uint8_t>& request, std::vector<uint8_t>& response);
    core::Result<void> handleGetAllKeys(uint64_t sessionId, std::vector<uint8_t>& response);
    core::Result<void> handleKeyExists(uint64_t sessionId, const std::string& key, std::vector<uint8_t>& response);
    core::Result<void> handleGetValue(uint64_t sessionId, const std::string& key, std::vector<uint8_t>& response);
    core::Result<void> handleSetValue(uint64_t sessionId, const std::string& key, const KvsDataType& value, std::vector<uint8_t>& response);
    core::Result<void> handleRemoveKey(uint64_t sessionId, const std::string& key, std::vector<uint8_t>& response);
    
    // Property management
    bool isPersistentKey(const std::string& key) const;
    bool isReadOnlyKey(const std::string& key) const;
    core::Result<void> loadPersistentProperties();
    core::Result<void> savePersistentProperty(const std::string& key, const PropertyEntry& entry);
    
    // Utility methods
    core::Result<void> createResponseMessage(uint64_t sessionId, int32_t operateId, int32_t errorCode, 
                                           const std::string& comment, std::vector<uint8_t>& response);
    EKvsDataTypeIndicate inferDataType(const KvsDataType& value) const;
    std::string propertyEntryToString(const PropertyEntry& entry) const;
    core::Result<PropertyEntry> stringToPropertyEntry(const std::string& value, EKvsDataTypeIndicate type) const;

private:
    // Socket and threading
    int m_serverFd;
    std::atomic<bool> m_running;
    std::thread m_serverThread;
    
    // Property storage
    std::unordered_map<std::string, PropertyEntry> m_properties;
    mutable boost::shared_mutex m_propertiesMutex;
    
    // Configuration
    std::string m_socketPath;
    std::string m_persistentDbPath;
    
    // Persistent storage backends using public interface
    core::SharedHandle<::lap::pm::CPersistencyManager> m_persistencyManager;
    core::SharedHandle<::lap::pm::KeyValueStorage> m_propertyStorage;
    core::SharedHandle<::lap::pm::KeyValueStorage> m_sqliteStorage;
    
    // LAP_PM_LOG_DECLARE();  // Commented out as not needed
};

} // namespace daemon
} // namespace pm
} // namespace lap

#endif // LAP_PERSISTENCY_DAEMON_CPERSISTENCYDAEMON_HPP_