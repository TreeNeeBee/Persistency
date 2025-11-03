#include "CPropertyClient.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <atomic>
#include <sstream>

#include <lap/log/CLog.hpp>
#include "CKvsPackedMessage.hpp"
#include "CPerErrorDomain.hpp"

namespace lap {
namespace pm {
namespace client {

std::atomic<uint64_t> PropertyClient::s_sessionCounter{1};

PropertyClient::PropertyClient(const std::string& socketPath)
    : m_socketPath(socketPath)
    , m_socketFd(-1)
    , m_connected(false)
{
}

PropertyClient::~PropertyClient() {
    disconnect();
}

core::Result<void> PropertyClient::connect() {
    if (m_connected) {
        return core::Result<void>::FromValue();
    }

    m_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_socketFd < 0) {
        LAP_PM_LOG_ERROR << "Failed to create socket: " << strerror(errno);
        return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(m_socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LAP_PM_LOG_ERROR << "Failed to connect to daemon: " << strerror(errno);
        close(m_socketFd);
        m_socketFd = -1;
        return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
    }

    m_connected = true;
    LAP_PM_LOG_DEBUG << "Connected to property daemon at: " << core::StringView(m_socketPath.c_str());
    return core::Result<void>::FromValue();
}

void PropertyClient::disconnect() {
    if (m_socketFd >= 0) {
        close(m_socketFd);
        m_socketFd = -1;
    }
    m_connected = false;
}

core::Result<std::string> PropertyClient::getProperty(const std::string& key, const std::string& defaultValue) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return core::Result<std::string>::FromError(connectResult.Error());
    }

    auto requestResult = createGetValueMessage(key);
    if (!requestResult.HasValue()) {
        return core::Result<std::string>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<std::string>::FromError(responseResult.Error());
    }

    auto valueResult = parseStringResponse(responseResult.Value());
    if (!valueResult.HasValue()) {
        // Return default value if key not found
        return core::Result<std::string>::FromValue(defaultValue);
    }

    return valueResult;
}

core::Result<void> PropertyClient::setProperty(const std::string& key, const std::string& value) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return connectResult;
    }

    KvsDataType kvsValue = value;
    auto requestResult = createSetValueMessage(key, kvsValue);
    if (!requestResult.HasValue()) {
        return core::Result<void>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<void>::FromError(responseResult.Error());
    }

    // Check response for success
    auto stringResult = parseStringResponse(responseResult.Value());
    LAP_PM_LOG_DEBUG << "Set property response: " << (stringResult.HasValue() ? core::StringView(stringResult.Value().c_str()) : core::StringView("error"));
    
    return core::Result<void>::FromValue();
}

core::Result<void> PropertyClient::setProperty(const std::string& key, int32_t value) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return connectResult;
    }

    KvsDataType kvsValue = value;
    auto requestResult = createSetValueMessage(key, kvsValue);
    if (!requestResult.HasValue()) {
        return core::Result<void>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<void>::FromError(responseResult.Error());
    }

    return core::Result<void>::FromValue();
}

core::Result<void> PropertyClient::setProperty(const std::string& key, bool value) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return connectResult;
    }

    KvsDataType kvsValue = value;
    auto requestResult = createSetValueMessage(key, kvsValue);
    if (!requestResult.HasValue()) {
        return core::Result<void>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<void>::FromError(responseResult.Error());
    }

    return core::Result<void>::FromValue();
}

core::Result<bool> PropertyClient::hasProperty(const std::string& key) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return core::Result<bool>::FromError(connectResult.Error());
    }

    auto requestResult = createKeyExistsMessage(key);
    if (!requestResult.HasValue()) {
        return core::Result<bool>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<bool>::FromError(responseResult.Error());
    }

    return parseBoolResponse(responseResult.Value());
}

core::Result<std::vector<std::string>> PropertyClient::getAllKeys() {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return core::Result<std::vector<std::string>>::FromError(connectResult.Error());
    }

    auto requestResult = createGetAllKeysMessage();
    if (!requestResult.HasValue()) {
        return core::Result<std::vector<std::string>>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<std::vector<std::string>>::FromError(responseResult.Error());
    }

    auto stringResult = parseStringResponse(responseResult.Value());
    if (!stringResult.HasValue()) {
        return core::Result<std::vector<std::string>>::FromError(stringResult.Error());
    }

    // Parse comma-separated keys
    std::vector<std::string> keys;
    std::string keysStr = stringResult.Value();
    if (!keysStr.empty()) {
        std::stringstream ss(keysStr);
        std::string key;
        while (std::getline(ss, key, ',')) {
            keys.push_back(key);
        }
    }

    return core::Result<std::vector<std::string>>::FromValue(keys);
}

core::Result<void> PropertyClient::removeProperty(const std::string& key) {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return connectResult;
    }

    auto requestResult = createRemoveKeyMessage(key);
    if (!requestResult.HasValue()) {
        return core::Result<void>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<void>::FromError(responseResult.Error());
    }

    return core::Result<void>::FromValue();
}

core::Result<void> PropertyClient::syncToStorage() {
    auto connectResult = connect();
    if (!connectResult.HasValue()) {
        return connectResult;
    }

    auto requestResult = createSyncMessage();
    if (!requestResult.HasValue()) {
        return core::Result<void>::FromError(requestResult.Error());
    }

    auto responseResult = sendMessage(requestResult.Value());
    if (!responseResult.HasValue()) {
        return core::Result<void>::FromError(responseResult.Error());
    }

    return core::Result<void>::FromValue();
}

core::Result<std::vector<uint8_t>> PropertyClient::sendMessage(const std::vector<uint8_t>& request) {
    if (!m_connected || m_socketFd < 0) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kPhysicalStorageFailure);
    }

    // Send request
    ssize_t bytesSent = send(m_socketFd, request.data(), request.size(), 0);
    if (bytesSent != static_cast<ssize_t>(request.size())) {
        LAP_PM_LOG_ERROR << "Failed to send complete request";
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kPhysicalStorageFailure);
    }

    // Receive response
    std::vector<uint8_t> response(4096);  // Max response size
    ssize_t bytesReceived = recv(m_socketFd, response.data(), response.size(), 0);
    if (bytesReceived <= 0) {
        LAP_PM_LOG_ERROR << "Failed to receive response: " << strerror(errno);
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kPhysicalStorageFailure);
    }

    response.resize(bytesReceived);
    return core::Result<std::vector<uint8_t>>::FromValue(response);
}

core::Result<std::vector<uint8_t>> PropertyClient::createGetAllKeysMessage() {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_GetAllKeys);
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::vector<uint8_t>> PropertyClient::createKeyExistsMessage(const std::string& key) {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_KeyExists);
    msg->set_key(key.c_str());
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::vector<uint8_t>> PropertyClient::createGetValueMessage(const std::string& key) {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_GetValue);
    msg->set_key(key.c_str());
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::vector<uint8_t>> PropertyClient::createSetValueMessage(const std::string& key, const KvsDataType& value) {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_SetValue);
    msg->set_key(key.c_str());
    
    // Set value based on type
    if (const auto* pInt8 = boost::get<core::Int8>(&value)) {
        msg->set_int8value(*pInt8);
    } else if (const auto* pUInt8 = boost::get<core::UInt8>(&value)) {
        msg->set_uint8value(*pUInt8);
    } else if (const auto* pInt16 = boost::get<core::Int16>(&value)) {
        msg->set_int16value(*pInt16);
    } else if (const auto* pUInt16 = boost::get<core::UInt16>(&value)) {
        msg->set_uint16value(*pUInt16);
    } else if (const auto* pInt32 = boost::get<core::Int32>(&value)) {
        msg->set_int32value(*pInt32);
    } else if (const auto* pUInt32 = boost::get<core::UInt32>(&value)) {
        msg->set_uint32value(*pUInt32);
    } else if (const auto* pInt64 = boost::get<core::Int64>(&value)) {
        msg->set_int64value(*pInt64);
    } else if (const auto* pUInt64 = boost::get<core::UInt64>(&value)) {
        msg->set_uint64value(*pUInt64);
    } else if (const auto* pBool = boost::get<core::Bool>(&value)) {
        msg->set_bvalue(*pBool);
    } else if (const auto* pFloat = boost::get<core::Float>(&value)) {
        msg->set_fvalue(*pFloat);
    } else if (const auto* pDouble = boost::get<core::Double>(&value)) {
        msg->set_dvalue(*pDouble);
    } else if (const auto* pString = boost::get<core::String>(&value)) {
        msg->set_strvalue(*pString);
    }
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::vector<uint8_t>> PropertyClient::createRemoveKeyMessage(const std::string& key) {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_RemoveKey);
    msg->set_key(key.c_str());
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::vector<uint8_t>> PropertyClient::createSyncMessage() {
    util::PackedMessage<kvs::kvsRemoteMsg> packedMsg;
    auto msg = packedMsg.msg();
    
    msg->set_sessionid(getNextSessionId());
    msg->set_operateid(kvs::kvsRemoteMsg_Operate_SyncToStorage);
    
    std::vector<uint8_t> buffer;
    if (!packedMsg.pack(buffer)) {
        return core::Result<std::vector<uint8_t>>::FromError(PerErrc::kValidationFailed);
    }
    
    return core::Result<std::vector<uint8_t>>::FromValue(buffer);
}

core::Result<std::string> PropertyClient::parseStringResponse(const std::vector<uint8_t>& response) {
    // Simple stub parsing - in real implementation would parse protobuf response
    if (response.empty()) {
        return core::Result<std::string>::FromError(PerErrc::kValidationFailed);
    }
    
    // For now, return a mock success response
    return core::Result<std::string>::FromValue("response_value");
}

core::Result<bool> PropertyClient::parseBoolResponse(const std::vector<uint8_t>& response) {
    auto stringResult = parseStringResponse(response);
    if (!stringResult.HasValue()) {
        return core::Result<bool>::FromError(stringResult.Error());
    }
    
    return core::Result<bool>::FromValue(stringResult.Value() == "true");
}

uint64_t PropertyClient::getNextSessionId() {
    return s_sessionCounter.fetch_add(1);
}

} // namespace client
} // namespace pm
} // namespace lap