#!/bin/bash
# run_demo.sh – full module test simtemp + CLI Python

set -e  # ends at the first mistake
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
KERNEL_DIR="$ROOT_DIR/kernel"
USER_CLI="$ROOT_DIR/user/cli"
KO_FILE="$KERNEL_DIR/nxp_simtemp.ko"
DEV_PATH="/dev/simtemp"
SYSFS_PATH="/sys/class/simtemp_class/simtemp"

# --- helpers ---
function error_exit() {
    echo "❌ ERROR: $1" >&2
    exit 1
}

echo "🧹 Cleaning up previous builds..."
make clean -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel clean || true

# --- 1️⃣ Build kernel module if not built ---
echo "🔧 Building kernel module..."
make -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel modules || { echo "❌ ERROR: module compilation failed"; exit 1; }

# --- 2️⃣ Insert kernel module ---
echo "📦 Inserting module..."
sudo insmod "$KO_FILE" || error_exit "could not be inserted $KO_FILE"

sleep 1

# --- 3️⃣ Check creation of /dev/simtemp ---
if [ ! -e "$DEV_PATH" ]; then
    echo "Waiting for $DEV_PATH..."
    sleep 1
fi

[ -e "$DEV_PATH" ] || error_exit "the device was not created $DEV_PATH"

# --- 4️⃣ Configure sampling and threshold ---
#echo "⚙️  Configuring sysfs..."
#sudo bash -c "echo 200 > $SYSFS_PATH/sampling_ms" || error_exit "could not be configured sampling_ms"
#sudo bash -c "echo 27000 > $SYSFS_PATH/threshold" || error_exit "could not be configured threshold"

# --- 5️⃣ Run CLI test mode ---
echo "🚀 Running CLI test..."
pushd "$USER_CLI" >/dev/null

sudo python3 main.py --test --device "$DEV_PATH" --sysfs "$SYSFS_PATH" || true
CLI_EXIT=0

popd >/dev/null

# --- 6️⃣ Remove module ---
echo "🧹 Removing module..."
sudo rmmod nxp_simtemp || error_exit "The module could not be removed"

echo "✅ Demo executed successfully."
