/**
 * @file CReplicaManager.hpp
 * @brief M-out-of-N replica management for data availability
 * @version 1.0
 * @date 2025-11-11
 * 
 * Implements redundancy strategy with multiple replicas:
 * - Writes to N replicas simultaneously
 * - Reads require M valid replicas for consensus
 * - Automatic repair of corrupted replicas
 */
#ifndef LAP_PERSISTENCY_REPLICAMANAGER_HPP
#define LAP_PERSISTENCY_REPLICAMANAGER_HPP

#include <lap/core/CResult.hpp>
#include <lap/core/CString.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CCrypto.hpp>
#include "CDataType.hpp"

namespace lap {
namespace per {

/**
 * @brief Replica status information
 */
struct ReplicaStatus {
    core::UInt32 replicaIndex;      // Replica number (0 to N-1)
    core::String replicaPath;       // Full path to replica
    core::Bool exists;              // Replica file exists
    core::Bool valid;               // Checksum validation passed
    core::String checksum;          // Current checksum value
    core::UInt64 fileSize;          // File size in bytes
    core::String lastModified;      // Last modification timestamp
};

/**
 * @brief Replica metadata for tracking
 */
struct ReplicaMetadata {
    core::String logicalFileName;   // Logical file name (without replica suffix)
    core::UInt32 totalReplicas;     // N: Total number of replicas
    core::UInt32 minValidReplicas;  // M: Minimum valid replicas required
    ChecksumType checksumType;      // Algorithm used for integrity checking
    core::String expectedChecksum;  // Expected checksum for all replicas
    core::Vector<ReplicaStatus> replicas; // Status of each replica
    core::String creationTime;      // First replica creation time
    core::String lastSyncTime;      // Last successful synchronization
};

/**
 * @brief M-out-of-N Replica Manager
 * 
 * Manages multiple replicas of each file for high availability:
 * - Write: Creates/updates all N replicas with integrity verification
 * - Read: Validates M-out-of-N replicas and returns consensus data
 * - Repair: Automatically fixes corrupted replicas from valid ones
 * 
 * File naming: {logicalName}.replica_0, {logicalName}.replica_1, ...
 */
class CReplicaManager {
public:
    /**
     * @brief Constructor
     * @param baseStoragePath Base path where replicas are stored
     * @param replicaCount Total number of replicas (N)
     * @param minValidReplicas Minimum valid replicas required (M)
     * @param checksumType Algorithm for integrity verification
     */
    explicit CReplicaManager(
        const core::String& baseStoragePath,
        core::UInt32 replicaCount = LAP_PER_DEFAULT_REPLICA_COUNT,
        core::UInt32 minValidReplicas = LAP_PER_MIN_VALID_REPLICAS,
        ChecksumType checksumType = ChecksumType::kCRC32
    ) noexcept;

    ~CReplicaManager() = default;

    // Disable copy, enable move
    CReplicaManager(const CReplicaManager&) = delete;
    CReplicaManager& operator=(const CReplicaManager&) = delete;
    CReplicaManager(CReplicaManager&&) = default;
    CReplicaManager& operator=(CReplicaManager&&) = default;

    /**
     * @brief Write data to all replicas
     * @param logicalFileName Logical file name (without replica suffix)
     * @param data Data buffer to write
     * @param size Size of data in bytes
     * @return Result with success or error
     * 
     * Creates N replicas and verifies each write with checksum.
     * Returns error if less than M replicas are successfully written.
     */
    core::Result<void> Write(
        const core::String& logicalFileName,
        const core::UInt8* data,
        core::Size size
    ) noexcept;

    /**
     * @brief Read data from replicas with M-out-of-N validation
     * @param logicalFileName Logical file name
     * @return Result containing data buffer or error
     * 
     * Reads from all replicas, validates checksums, and returns data
     * if at least M replicas agree (same checksum). Automatically
     * repairs corrupted replicas if M valid ones are found.
     */
    core::Result<core::Vector<core::UInt8>> Read(
        const core::String& logicalFileName
    ) noexcept;

    /**
     * @brief Delete all replicas of a logical file
     * @param logicalFileName Logical file name
     * @return Result with success or error
     */
    core::Result<void> Delete(
        const core::String& logicalFileName
    ) noexcept;

    /**
     * @brief Check replica status and health
     * @param logicalFileName Logical file name
     * @return Result containing ReplicaMetadata with detailed status
     */
    core::Result<ReplicaMetadata> CheckStatus(
        const core::String& logicalFileName
    ) noexcept;

    /**
     * @brief Repair corrupted replicas from valid ones
     * @param logicalFileName Logical file name
     * @return Result with number of repaired replicas or error
     * 
     * Requires at least M valid replicas to perform repair.
     */
    core::Result<core::UInt32> Repair(
        const core::String& logicalFileName
    ) noexcept;

    /**
     * @brief Get list of all logical files (without replica suffixes)
     * @return Result containing vector of logical file names
     */
    core::Result<core::Vector<core::String>> ListFiles() noexcept;

    /**
     * @brief Reconfigure replica parameters
     * @param replicaCount New total replicas (N)
     * @param minValidReplicas New minimum valid (M)
     * @return Result with success or error
     */
    core::Result<void> Reconfigure(
        core::UInt32 replicaCount,
        core::UInt32 minValidReplicas
    ) noexcept;

    /**
     * @brief Get base storage path
     */
    inline const core::String& GetBaseStoragePath() const noexcept {
        return m_baseStoragePath;
    }

    /**
     * @brief Get replica configuration
     */
    inline core::UInt32 GetReplicaCount() const noexcept {
        return m_replicaCount;
    }

    inline core::UInt32 GetMinValidReplicas() const noexcept {
        return m_minValidReplicas;
    }

private:
    /**
     * @brief Generate replica file path
     * @param logicalFileName Logical file name
     * @param replicaIndex Replica index (0 to N-1)
     * @return Full path to replica file
     */
    core::String GetReplicaPath(
        const core::String& logicalFileName,
        core::UInt32 replicaIndex
    ) const noexcept;

    /**
     * @brief Extract logical file name from replica path
     * @param replicaPath Full replica path
     * @return Logical file name or empty if not a replica
     */
    core::Result<core::String> ExtractLogicalName(
        const core::String& replicaPath
    ) const noexcept;

    /**
     * @brief Validate all replicas and return status
     */
    core::Result<ReplicaMetadata> ValidateAllReplicas(
        const core::String& logicalFileName
    ) noexcept;

    /**
     * @brief Find consensus among valid replicas
     * @return Result with checksum that has M or more matches
     */
    core::Result<core::String> FindConsensusChecksum(
        const core::Vector<ReplicaStatus>& replicas
    ) const noexcept;

    /**
     * @brief Write single replica
     */
    core::Result<void> WriteReplica(
        const core::String& replicaPath,
        const core::UInt8* data,
        core::Size size,
        const core::String& expectedChecksum
    ) noexcept;

    /**
     * @brief Read single replica with validation
     */
    core::Result<core::Vector<core::UInt8>> ReadReplica(
        const core::String& replicaPath,
        const core::String& expectedChecksum
    ) noexcept;

    // Member variables
    core::String m_baseStoragePath;     // Base path for all replicas
    core::UInt32 m_replicaCount;        // N: Total replicas
    core::UInt32 m_minValidReplicas;    // M: Minimum valid required
    ChecksumType m_checksumType;        // Checksum algorithm
};

} // namespace per
} // namespace lap

#endif // LAP_PERSISTENCY_REPLICAMANAGER_HPP
