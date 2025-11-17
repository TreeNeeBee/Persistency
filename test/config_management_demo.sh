#!/bin/bash
# Persistency Configuration Management with Core/tools/config_editor.py
# This script demonstrates how to manage Persistency module configuration

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
CONFIG_EDITOR="$ROOT_DIR/modules/Core/tools/config_editor.py"
CONFIG_FILE="$ROOT_DIR/config.json"

# Set HMAC secret for config integrity
export HMAC_SECRET="test_secret_key"

echo "=========================================="
echo "Persistency Configuration Management Demo"
echo "=========================================="
echo

# 1. Display current configuration
echo "1. Current Persistency Configuration:"
echo "------------------------------------"
python3 "$CONFIG_EDITOR" "$CONFIG_FILE" --display 2>&1 | grep -A 20 "persistency:" || echo "No persistency config found"
echo

# 2. Modify Property Backend configuration
echo "2. Modifying Property Backend Settings:"
echo "---------------------------------------"
echo "  - Setting propertyBackendShmSize to 4MB (4194304 bytes)"
echo "  - Setting propertyBackendPersistence to 'sqlite'"
python3 "$CONFIG_EDITOR" "$CONFIG_FILE" \
    --set persistency.propertyBackendShmSize=4194304 \
    --set persistency.propertyBackendPersistence="sqlite"
echo

# 3. Enable advanced features
echo "3. Enabling Advanced Features:"
echo "------------------------------"
echo "  - Enabling WAL mode for SQLite"
echo "  - Enabling transactions"
echo "  - Setting cache size to 4000 pages"
python3 "$CONFIG_EDITOR" "$CONFIG_FILE" \
    --set persistency.enableWAL=true \
    --set persistency.enableTransactions=true \
    --set persistency.cacheSize=4000
echo

# 4. Verify configuration integrity
echo "4. Verifying Configuration Integrity:"
echo "-------------------------------------"
if python3 "$CONFIG_EDITOR" "$CONFIG_FILE" --verify 2>&1 | grep -q "✓"; then
    echo "✓ Configuration integrity verified successfully"
else
    echo "✗ Configuration integrity verification failed"
    exit 1
fi
echo

# 5. Display updated configuration
echo "5. Updated Persistency Configuration:"
echo "------------------------------------"
python3 "$CONFIG_EDITOR" "$CONFIG_FILE" --display 2>&1 | grep -A 20 "persistency:"
echo

# 6. Run tests with new configuration
echo "6. Running Persistency Tests with Updated Config:"
echo "-------------------------------------------------"
cd "$ROOT_DIR/build"
if [ -f "./modules/Persistency/persistency_test" ]; then
    ./modules/Persistency/persistency_test --gtest_filter="CoreConstraintsTest.*" 2>&1 | grep -E "PASSED|FAILED"
    echo "✓ Tests completed"
else
    echo "⚠ Test binary not found. Run 'make persistency_test' first."
fi

echo
echo "=========================================="
echo "Demo completed successfully!"
echo "=========================================="
