#!/bin/bash
# Demo script to show the improved shared memory naming

echo "=== Shared Memory Naming Improvement Demo ==="
echo ""
echo "Before improvement:"
echo "  Format: shm_kvs_<hash>"
echo "  Example: shm_kvs_a3f5b8c2"
echo "  Problems: Cannot debug, may conflict, hard to cleanup"
echo ""
echo "After improvement:"
echo "  Format: shm_kvs_<pid>_<sanitized_name>_<hash>"
echo "  Example: shm_kvs_12345_my_storage_file_a3f5b8c2"
echo "  Benefits:"
echo "    ✓ Process isolation (different PIDs)"
echo "    ✓ Easy debugging (readable name)"
echo "    ✓ Simple cleanup (by PID)"
echo ""
echo "Running a quick test..."
echo ""

cd "$(dirname "$0")/../build"

# Run test
./modules/Persistency/persistency_test --gtest_filter="PropertyBackendTest.Constructor_ShouldInitializeSuccessfully" 2>&1 > /dev/null

# Check shared memory
echo "Current shared memory segments:"
ipcs -m | head -3
ipcs -m | grep "shm_kvs" | head -5 || echo "  (No persistent segments - cleaned up after test)"

echo ""
echo "=== Demo Complete ==="
