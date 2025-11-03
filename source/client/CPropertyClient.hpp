#ifndef LAP_PERSISTENCY_CLIENT_CPROPERTYCLIENT_HPP_
#define LAP_PERSISTENCY_CLIENT_CPROPERTYCLIENT_HPP_

#include <string>
#include <vector>
#include <memory>

#include <lap/core/CTypedef.hpp>
#include <lap/core/CResult.hpp>

#include "CDataType.hpp"

namespace lap {
namespace pm {
namespace client {

class PropertyClient {
public:
    PropertyClient(const std::string& socketPath = "/tmp/property_service");
    ~PropertyClient();

    // Connect to property daemon
    core::Result<void> connect();
    void disconnect();

    // Property operations (Android property style)
    core::Result<std::string> getProperty(const std::string& key, const std::string& defaultValue = "");
    core::Result<void> setProperty(const std::string& key, const std::string& value);
    core::Result<void> setProperty(const std::string& key, int32_t value);
    core::Result<void> setProperty(const std::string& key, bool value);
    
    // Additional operations
    core::Result<bool> hasProperty(const std::string& key);
    core::Result<std::vector<std::string>> getAllKeys();
    core::Result<void> removeProperty(const std::string& key);
    core::Result<void> syncToStorage();

private:
    std::string m_socketPath;
    int m_socketFd;
    bool m_connected;
    
    // Internal communication methods
    core::Result<std::vector<uint8_t>> sendMessage(const std::vector<uint8_t>& request);
    core::Result<std::vector<uint8_t>> createGetAllKeysMessage();
    core::Result<std::vector<uint8_t>> createKeyExistsMessage(const std::string& key);
    core::Result<std::vector<uint8_t>> createGetValueMessage(const std::string& key);
    core::Result<std::vector<uint8_t>> createSetValueMessage(const std::string& key, const KvsDataType& value);
    core::Result<std::vector<uint8_t>> createRemoveKeyMessage(const std::string& key);
    core::Result<std::vector<uint8_t>> createSyncMessage();
    
    core::Result<std::string> parseStringResponse(const std::vector<uint8_t>& response);
    core::Result<bool> parseBoolResponse(const std::vector<uint8_t>& response);
    
    uint64_t getNextSessionId();
    
    static std::atomic<uint64_t> s_sessionCounter;
};

} // namespace client
} // namespace pm
} // namespace lap

#endif // LAP_PERSISTENCY_CLIENT_CPROPERTYCLIENT_HPP_