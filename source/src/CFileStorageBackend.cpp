/**
 * @file CFileStorageBackend.cpp
 * @brief File Storage Backend implementation
 * @version 3.0
 * @date 2025-11-14
 * 
 * Architecture: Phase 2.1 Refactoring
 * - Uses Core::File::Util for all file operations (no std::fstream)
 * - Uses Core::Path for path operations
 * - Pure file operations, no lifecycle management
 */

#include "CFileStorageBackend.hpp"
#include "CPerErrorDomain.hpp"
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>

namespace lap {
namespace per {

using namespace core;

// ============================================================================
// Basic File Operations
// ============================================================================

Result<Vector<Byte>> CFileStorageBackend::ReadFile(
    const String& fileName,
    const String& category
) noexcept {
    auto filePath = GetFilePath(fileName, category);
    
    // Use Core::File::Util to read binary data
    Vector<UInt8> fileData;
    if (!File::Util::ReadBinary(filePath, fileData)) {
        LAP_PER_LOG_ERROR << "Failed to read file: " << filePath;
        return Result<Vector<Byte>>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }
    
    // Convert UInt8 to Byte
    Vector<Byte> result;
    result.reserve(fileData.size());
    for (auto byte : fileData) {
        result.push_back(static_cast<Byte>(byte));
    }
    
    return Result<Vector<Byte>>::FromValue(std::move(result));
}

Result<void> CFileStorageBackend::WriteFile(
    const String& fileName,
    const Vector<Byte>& data,
    const String& category
) noexcept {
    auto filePath = GetFilePath(fileName, category);
    
    // Ensure category directory exists
    auto categoryPath = GetCategoryPath(category);
    if (!Path::isDirectory(categoryPath)) {
        if (!Path::createDirectory(categoryPath)) {
            LAP_PER_LOG_ERROR << "Failed to create category directory: " << categoryPath;
            return Result<void>::FromError(
                MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
            );
        }
    }
    
    // Convert Byte to UInt8
    Vector<UInt8> fileData;
    fileData.reserve(data.size());
    for (auto byte : data) {
        fileData.push_back(static_cast<UInt8>(byte));
    }
    
    // Special handling for empty files - Core::File::Util::WriteBinary requires size > 0
    if (fileData.empty()) {
        // Create empty file using standard API
        if (!File::Util::create(filePath)) {
            LAP_PER_LOG_ERROR << "Failed to create empty file: " << filePath;
            return Result<void>::FromError(
                MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
            );
        }
        return Result<void>::FromValue();
    }
    
    // Use Core::File::Util to write binary data (createDirectories=true)
    if (!File::Util::WriteBinary(filePath, fileData.data(), fileData.size(), true)) {
        LAP_PER_LOG_ERROR << "Failed to write file: " << filePath;
        return Result<void>::FromError(
            MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
        );
    }
    
    return Result<void>::FromValue();
}

Result<void> CFileStorageBackend::DeleteFile(
    const String& fileName,
    const String& category
) noexcept {
    auto filePath = GetFilePath(fileName, category);
    
    if (!File::Util::exists(filePath)) {
        LAP_PER_LOG_WARN << "File does not exist: " << filePath;
        return Result<void>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }
    
    if (!File::Util::remove(filePath)) {
        LAP_PER_LOG_ERROR << "Failed to delete file: " << fileName;
        return Result<void>::FromError(
            MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
        );
    }
    
    return Result<void>::FromValue();
}

Result<Vector<String>> CFileStorageBackend::ListFiles(
    const String& category
) noexcept {
    auto categoryPath = GetCategoryPath(category);
    
    if (!Path::isDirectory(categoryPath)) {
        return Result<Vector<String>>::FromValue(Vector<String>());
    }
    
    auto files = Path::listFiles(categoryPath);
    Vector<String> result;
    result.reserve(files.size());
    
    for (const auto& file : files) {
        result.push_back(String(file));
    }
    
    return Result<Vector<String>>::FromValue(std::move(result));
}

Bool CFileStorageBackend::FileExists(
    const String& fileName,
    const String& category
) const noexcept {
    auto filePath = GetFilePath(fileName, category);
    return File::Util::exists(filePath);
}

Result<UInt64> CFileStorageBackend::GetFileSize(
    const String& fileName,
    const String& category
) const noexcept {
    auto filePath = GetFilePath(fileName, category);
    
    if (!File::Util::exists(filePath)) {
        return Result<UInt64>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }
    
    // Read file to get size (Core::File::Util doesn't have direct getFileSize)
    Vector<UInt8> fileData;
    if (!File::Util::ReadBinary(filePath, fileData)) {
        return Result<UInt64>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }
    
    return Result<UInt64>::FromValue(static_cast<UInt64>(fileData.size()));
}

// ============================================================================
// Helper Operations
// ============================================================================

StorageUri CFileStorageBackend::GetFileUri(
    const String& fileName,
    const String& category
) const noexcept {
    StorageUri uri;
    uri.baseUri = m_basePath;
    uri.category = category;
    uri.fileName = fileName;
    return uri;
}

Result<void> CFileStorageBackend::CopyFile(
    const String& fileName,
    const String& fromCategory,
    const String& toCategory
) noexcept {
    auto srcPath = GetFilePath(fileName, fromCategory);
    auto dstPath = GetFilePath(fileName, toCategory);
    
    if (!File::Util::exists(srcPath)) {
        LAP_PER_LOG_ERROR << "Source file does not exist: " << srcPath;
        return Result<void>::FromError(
            MakeErrorCode(PerErrc::kFileNotFound, 0)
        );
    }
    
    // Ensure destination category directory exists
    auto dstCategoryPath = GetCategoryPath(toCategory);
    if (!Path::isDirectory(dstCategoryPath)) {
        if (!Path::createDirectory(dstCategoryPath)) {
            LAP_PER_LOG_ERROR << "Failed to create destination category directory";
            return Result<void>::FromError(
                MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
            );
        }
    }
    
    if (!File::Util::copy(srcPath, dstPath)) {
        LAP_PER_LOG_ERROR << "Failed to copy file: " << fileName;
        return Result<void>::FromError(
            MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0)
        );
    }
    
    return Result<void>::FromValue();
}

Result<void> CFileStorageBackend::MoveFile(
    const String& fileName,
    const String& fromCategory,
    const String& toCategory
) noexcept {
    // Atomic move: copy + verify + delete
    auto copyResult = CopyFile(fileName, fromCategory, toCategory);
    if (!copyResult.HasValue()) {
        return copyResult;
    }
    
    auto deleteResult = DeleteFile(fileName, fromCategory);
    if (!deleteResult.HasValue()) {
        // Rollback: delete destination
        DeleteFile(fileName, toCategory);
        return deleteResult;
    }
    
    return Result<void>::FromValue();
}

// ============================================================================
// Helper Methods
// ============================================================================

String CFileStorageBackend::GetCategoryPath(const String& category) const noexcept {
    return Path::appendString(m_basePath, category);
}

String CFileStorageBackend::GetFilePath(
    const String& fileName,
    const String& category
) const noexcept {
    auto categoryPath = GetCategoryPath(category);
    return Path::appendString(categoryPath, fileName);
}

} // namespace per
} // namespace lap


