/**
 * @file CReplicaManager.cpp
 * @brief Implementation of M-out-of-N replica management
 * @version 1.0
 * @date 2025-11-11
 */

#include "CReplicaManager.hpp"
#include <lap/core/CCore.hpp>
#include <lap/core/CTime.hpp>
#include <lap/log/CLog.hpp>
#include <algorithm>
#include <map>

namespace lap {
namespace per {

using namespace core;

// Helper functions for checksum operations (using Core::Crypto utilities)

static inline String ChecksumTypeToString(ChecksumType type) noexcept {
    switch (type) {
        case ChecksumType::kCRC32:  return "CRC32";
        case ChecksumType::kSHA256: return "SHA256";
        default: return "Unknown";
    }
}

static inline String UInt32ToHex(UInt32 value) noexcept {
    // Convert UInt32 to bytes (big-endian) and use Core::Crypto::Util::bytesToHex
    UInt8 bytes[4] = {
        static_cast<UInt8>((value >> 24) & 0xFF),
        static_cast<UInt8>((value >> 16) & 0xFF),
        static_cast<UInt8>((value >> 8) & 0xFF),
        static_cast<UInt8>(value & 0xFF)
    };
    return Crypto::Util::bytesToHex(bytes, 4);
}

static Result<ChecksumResult> CalculateBuffer(
    const UInt8* data,
    Size size,
    ChecksumType type
) noexcept {
    if (!data || size == 0) {
        return Result<ChecksumResult>::FromError(
            MakeErrorCode(PerErrc::kInvalidArgument, 0)
        );
    }

    auto startTime = Time::getCurrentTime();
    ChecksumResult result;
    result.type = type;

    if (type == ChecksumType::kCRC32) {
        UInt32 crc32 = Crypto::Util::computeCrc32(data, size);
        result.value = UInt32ToHex(crc32);
    } else if (type == ChecksumType::kSHA256) {
        result.value = Crypto::Util::computeSha256(data, size);
    } else {
        return Result<ChecksumResult>::FromError(
            MakeErrorCode(PerErrc::kInvalidArgument, 0)
        );
    }

    auto endTime = Time::getCurrentTime();
    result.calculationTime = endTime - startTime;

    return Result<ChecksumResult>::FromValue(std::move(result));
}

static Result<ChecksumResult> CalculateFile(
    const String& filePath,
    ChecksumType type
) noexcept {
    auto startTime = Time::getCurrentTime();

    // Read file content
    Vector<UInt8> content;
    if (!File::Util::ReadBinary(filePath, content)) {
        return Result<ChecksumResult>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }

    auto result = CalculateBuffer(content.data(), content.size(), type);

    if (result.HasValue()) {
        // Update calculation time
        auto endTime = Time::getCurrentTime();
        ChecksumResult updatedResult = result.Value();
        updatedResult.calculationTime = endTime - startTime;
        return Result<ChecksumResult>::FromValue(std::move(updatedResult));
    }

    return result;
}

static Result<Bool> VerifyFile(
    const String& filePath,
    const String& expectedChecksum,
    ChecksumType type
) noexcept {
    auto checksumResult = CalculateFile(filePath, type);
    if (!checksumResult.HasValue()) {
        return Result<Bool>::FromError(checksumResult.Error());
    }

    Bool matches = (checksumResult.Value().value == expectedChecksum);
    return Result<Bool>::FromValue(matches);
}

// CReplicaManager implementation

CReplicaManager::CReplicaManager(
    const String& baseStoragePath,
    UInt32 replicaCount,
    UInt32 minValidReplicas,
    ChecksumType checksumType
) noexcept
    : m_baseStoragePath(baseStoragePath)
    , m_replicaCount(replicaCount)
    , m_minValidReplicas(minValidReplicas)
    , m_checksumType(checksumType)
{
    // Validate configuration
    if (m_minValidReplicas > m_replicaCount) {
        LAP_PER_LOG_WARN << "MinValidReplicas (" << m_minValidReplicas 
                         << ") > ReplicaCount (" << m_replicaCount 
                         << "), adjusting to match";
        m_minValidReplicas = m_replicaCount;
    }

    if (m_minValidReplicas == 0) {
        LAP_PER_LOG_WARN << "MinValidReplicas is 0, setting to 1";
        m_minValidReplicas = 1;
    }

    // Ensure base storage path exists
    if (!Path::createDirectory(StringView(m_baseStoragePath))) {
        LAP_PER_LOG_ERROR << "Failed to create base storage path: " << m_baseStoragePath;
    }

    LAP_PER_LOG_INFO << "ReplicaManager initialized: " 
                     << "N=" << m_replicaCount 
                     << ", M=" << m_minValidReplicas
                     << ", Checksum=" << ChecksumTypeToString(m_checksumType)
                     << ", Path=" << m_baseStoragePath;
}

String CReplicaManager::GetReplicaPath(
    const String& logicalFileName,
    UInt32 replicaIndex
) const noexcept {
    String replicaSuffix = String(".") + LAP_PER_REPLICA_DIR_PREFIX + StringUtil::FromInt(replicaIndex);
    return Path::appendString(StringView(m_baseStoragePath), StringView(logicalFileName + replicaSuffix));
}

Result<String> CReplicaManager::ExtractLogicalName(
    const String& replicaPath
) const noexcept {
    String fileName = Path::basename(replicaPath);
    
    // Find ".replica_" pattern
    String pattern = String(".") + LAP_PER_REPLICA_DIR_PREFIX;
    auto pos = fileName.find(pattern);
    
    if (pos == String::npos) {
        LAP_PER_LOG_ERROR << "Not a replica file: " << fileName;
        return Result<String>::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
    }

    String logicalName = fileName.substr(0, pos);
    return Result<String>::FromValue(std::move(logicalName));
}

Result<void> CReplicaManager::WriteReplica(
    const String& replicaPath,
    const UInt8* data,
    Size size,
    const String& expectedChecksum
) noexcept {
    // Write data to file
    if (!File::Util::WriteBinary(replicaPath, data, size)) {
        LAP_PER_LOG_ERROR << "Failed to write replica: " << replicaPath;
        return Result<void>::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
    }

    // Verify written data
    auto verifyResult = VerifyFile(
        replicaPath,
        expectedChecksum,
        m_checksumType
    );

    if (!verifyResult.HasValue()) {
        // Delete corrupted file
        File::Util::remove(replicaPath);
        LAP_PER_LOG_ERROR << "Replica verification failed";
        return Result<void>::FromError(verifyResult.Error());
    }

    if (!verifyResult.Value()) {
        File::Util::remove(replicaPath);
        LAP_PER_LOG_ERROR << "Replica checksum mismatch after write";
        return Result<void>::FromError(MakeErrorCode(PerErrc::kChecksumMismatch, 0));
    }

    return Result<void>::FromValue();
}

Result<Vector<UInt8>> CReplicaManager::ReadReplica(
    const String& replicaPath,
    const String& expectedChecksum
) noexcept {
    // Verify checksum first
    auto verifyResult = VerifyFile(
        replicaPath,
        expectedChecksum,
        m_checksumType
    );

    if (!verifyResult.HasValue() || !verifyResult.Value()) {
        LAP_PER_LOG_ERROR << "Replica checksum verification failed: " << replicaPath;
        return Result<Vector<UInt8>>::FromError(MakeErrorCode(PerErrc::kChecksumMismatch, 0));
    }

    // Read file content
    Vector<UInt8> data;
    if (!File::Util::ReadBinary(replicaPath, data)) {
        LAP_PER_LOG_ERROR << "Failed to read replica: " << replicaPath;
        return Result<Vector<UInt8>>::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
    }

    return Result<Vector<UInt8>>::FromValue(std::move(data));
}

Result<void> CReplicaManager::Write(
    const String& logicalFileName,
    const UInt8* data,
    Size size
) noexcept {
    if (!data || size == 0) {
        LAP_PER_LOG_ERROR << "Invalid data or size for replica write";
        return Result<void>::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
    }

    LAP_PER_LOG_DEBUG << "Writing replicas for: " << logicalFileName 
                      << " (" << size << " bytes)";

    // Calculate checksum once
    auto checksumResult = CalculateBuffer(data, size, m_checksumType);
    if (!checksumResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to calculate checksum: " << checksumResult.Error().Message();
        return Result<void>::FromError(checksumResult.Error());
    }

    String expectedChecksum = checksumResult.Value().value;

    // Write to all replicas
    UInt32 successCount = 0;
    String lastError;

    for (UInt32 i = 0; i < m_replicaCount; ++i) {
        String replicaPath = GetReplicaPath(logicalFileName, i);
        
        auto result = WriteReplica(replicaPath, data, size, expectedChecksum);
        if (result.HasValue()) {
            ++successCount;
            LAP_PER_LOG_VERBOSE << "Replica " << i << " written successfully: " << replicaPath;
        } else {
            lastError = result.Error().Message();
            LAP_PER_LOG_ERROR << "Failed to write replica " << i << ": " << lastError;
        }
    }

    // Check if we met minimum requirement
    if (successCount < m_minValidReplicas) {
        LAP_PER_LOG_ERROR << "Only " << successCount << " of " << m_minValidReplicas 
                          << " required replicas written. Last error: " << lastError;
        return Result<void>::FromError(MakeErrorCode(PerErrc::kOutOfStorageSpace, 0));
    }

    LAP_PER_LOG_INFO << "Successfully wrote " << successCount << "/" << m_replicaCount 
                     << " replicas for: " << logicalFileName;

    return Result<void>::FromValue();
}

Result<ReplicaMetadata> CReplicaManager::ValidateAllReplicas(
    const String& logicalFileName
) noexcept {
    ReplicaMetadata metadata;
    metadata.logicalFileName = logicalFileName;
    metadata.totalReplicas = m_replicaCount;
    metadata.minValidReplicas = m_minValidReplicas;
    metadata.checksumType = m_checksumType;
    metadata.creationTime = core::Time::getCurrentTime();

    // Check each replica
    for (UInt32 i = 0; i < m_replicaCount; ++i) {
        ReplicaStatus status;
        status.replicaIndex = i;
        status.replicaPath = GetReplicaPath(logicalFileName, i);
        
        // Check if file exists
        status.exists = File::Util::exists(status.replicaPath);

        if (status.exists) {
            // Get file info
            status.fileSize = File::Util::size(status.replicaPath);
            status.lastModified = File::Util::getModificationTime(status.replicaPath);

            // Calculate checksum
            auto checksumResult = CalculateFile(
                status.replicaPath,
                m_checksumType
            );

            if (checksumResult.HasValue()) {
                status.checksum = checksumResult.Value().value;
                status.valid = true;
            } else {
                status.valid = false;
                LAP_PER_LOG_WARN << "Replica " << i << " checksum failed: " 
                                 << checksumResult.Error().Message();
            }
        } else {
            status.valid = false;
            status.fileSize = 0;
        }

        metadata.replicas.push_back(status);
    }

    return Result<ReplicaMetadata>::FromValue(std::move(metadata));
}

Result<String> CReplicaManager::FindConsensusChecksum(
    const Vector<ReplicaStatus>& replicas
) const noexcept {
    // Count occurrences of each checksum
    std::map<std::string, UInt32> checksumCount;

    for (const auto& replica : replicas) {
        if (replica.valid && replica.exists) {
            checksumCount[std::string(replica.checksum)]++;
        }
    }

    // Find checksum with >= M matches
    for (const auto& [checksum, count] : checksumCount) {
        if (count >= m_minValidReplicas) {
            return Result<String>::FromValue(String(checksum.c_str()));
        }
    }

    return Result<String>::FromError(
        MakeErrorCode(PerErrc::kIntegrityCorrupted, 0)
    );
}

Result<Vector<UInt8>> CReplicaManager::Read(
    const String& logicalFileName
) noexcept {
    LAP_PER_LOG_DEBUG << "Reading replicas for: " << logicalFileName;

    // Validate all replicas
    auto metadataResult = ValidateAllReplicas(logicalFileName);
    if (!metadataResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to validate replicas: " << metadataResult.Error().Message();
        return Result<Vector<UInt8>>::FromError(metadataResult.Error());
    }

    auto& metadata = metadataResult.Value();

    // Find consensus checksum
    auto consensusResult = FindConsensusChecksum(metadata.replicas);
    if (!consensusResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to achieve consensus: " << consensusResult.Error().Message();
        return Result<Vector<UInt8>>::FromError(consensusResult.Error());
    }

    String consensusChecksum = consensusResult.Value();
    LAP_PER_LOG_DEBUG << "Consensus checksum: " << consensusChecksum;

    // Read from first valid replica with matching checksum
    for (const auto& replica : metadata.replicas) {
        if (replica.valid && replica.checksum == consensusChecksum) {
            auto readResult = ReadReplica(replica.replicaPath, consensusChecksum);
            if (readResult.HasValue()) {
                LAP_PER_LOG_INFO << "Successfully read from replica " << replica.replicaIndex;
                
                // Trigger background repair if needed
                UInt32 validCount = 0;
                for (const auto& r : metadata.replicas) {
                    if (r.valid && r.checksum == consensusChecksum) ++validCount;
                }
                
                if (validCount < m_replicaCount) {
                    LAP_PER_LOG_WARN << "Only " << validCount << "/" << m_replicaCount 
                                     << " replicas valid, repair recommended";
                }

                return readResult;
            }
        }
    }

    return Result<Vector<UInt8>>::FromError(
        MakeErrorCode(PerErrc::kFileNotFound, 0)
    );
}

Result<void> CReplicaManager::Delete(
    const String& logicalFileName
) noexcept {
    LAP_PER_LOG_DEBUG << "Deleting all replicas for: " << logicalFileName;

    UInt32 deletedCount = 0;
    for (UInt32 i = 0; i < m_replicaCount; ++i) {
        String replicaPath = GetReplicaPath(logicalFileName, i);
        if (File::Util::remove(replicaPath)) {
            ++deletedCount;
        }
    }

    LAP_PER_LOG_INFO << "Deleted " << deletedCount << "/" << m_replicaCount 
                     << " replicas for: " << logicalFileName;

    return Result<void>::FromValue();
}

Result<ReplicaMetadata> CReplicaManager::CheckStatus(
    const String& logicalFileName
) noexcept {
    return ValidateAllReplicas(logicalFileName);
}

Result<UInt32> CReplicaManager::Repair(
    const String& logicalFileName
) noexcept {
    LAP_PER_LOG_INFO << "Repairing replicas for: " << logicalFileName;

    // Validate and find consensus
    auto metadataResult = ValidateAllReplicas(logicalFileName);
    if (!metadataResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Failed to validate replicas for repair: " << metadataResult.Error().Message();
        return Result<UInt32>::FromError(metadataResult.Error());
    }

    auto& metadata = metadataResult.Value();
    auto consensusResult = FindConsensusChecksum(metadata.replicas);
    if (!consensusResult.HasValue()) {
        LAP_PER_LOG_ERROR << "Cannot repair: " << consensusResult.Error().Message();
        return Result<UInt32>::FromError(consensusResult.Error());
    }

    String consensusChecksum = consensusResult.Value();

    // Read valid data
    Vector<UInt8> validData;
    for (const auto& replica : metadata.replicas) {
        if (replica.valid && replica.checksum == consensusChecksum) {
            auto readResult = ReadReplica(replica.replicaPath, consensusChecksum);
            if (readResult.HasValue()) {
                validData = std::move(readResult.Value());
                break;
            }
        }
    }

    if (validData.empty()) {
        LAP_PER_LOG_ERROR << "No valid data found for repair";
        return Result<UInt32>::FromError(MakeErrorCode(PerErrc::kFileNotFound, 0));
    }

    // Repair invalid replicas
    UInt32 repairedCount = 0;
    for (const auto& replica : metadata.replicas) {
        if (!replica.valid || replica.checksum != consensusChecksum) {
            auto result = WriteReplica(
                replica.replicaPath,
                validData.data(),
                validData.size(),
                consensusChecksum
            );

            if (result.HasValue()) {
                ++repairedCount;
                LAP_PER_LOG_INFO << "Repaired replica " << replica.replicaIndex;
            } else {
                LAP_PER_LOG_ERROR << "Failed to repair replica " << replica.replicaIndex 
                                  << ": " << result.Error().Message();
            }
        }
    }

    LAP_PER_LOG_INFO << "Repaired " << repairedCount << " replicas for: " << logicalFileName;

    return Result<UInt32>::FromValue(repairedCount);
}

Result<Vector<String>> CReplicaManager::ListFiles() noexcept {
    Vector<String> allFiles = Path::listFiles(StringView(m_baseStoragePath));

    // Extract unique logical names
    std::set<std::string> logicalNames;
    for (const auto& fileName : allFiles) {
        auto nameResult = ExtractLogicalName(fileName);
        if (nameResult.HasValue()) {
            logicalNames.insert(std::string(nameResult.Value()));
        }
    }

    Vector<String> files;
    for (const auto& name : logicalNames) {
        files.push_back(String(name.c_str()));
    }

    return Result<Vector<String>>::FromValue(std::move(files));
}

Result<void> CReplicaManager::Reconfigure(
    UInt32 replicaCount,
    UInt32 minValidReplicas
) noexcept {
    if (minValidReplicas > replicaCount) {
        LAP_PER_LOG_ERROR << "MinValidReplicas cannot exceed ReplicaCount";
        return Result<void>::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
    }

    if (minValidReplicas == 0) {
        LAP_PER_LOG_ERROR << "MinValidReplicas must be at least 1";
        return Result<void>::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
    }

    LAP_PER_LOG_INFO << "Reconfiguring ReplicaManager: N=" << m_replicaCount 
                     << "->" << replicaCount 
                     << ", M=" << m_minValidReplicas 
                     << "->" << minValidReplicas;

    m_replicaCount = replicaCount;
    m_minValidReplicas = minValidReplicas;

    return Result<void>::FromValue();
}

} // namespace per
} // namespace lap
