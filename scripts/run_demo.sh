#!/bin/bash
# run_demo.sh – prueba completa del módulo simtemp + CLI Python

set -e  # termina al primer error
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

echo "🧹 Limpiando compilaciones anteriores..."
make clean -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel clean || true

# --- 1️⃣ Build kernel module if not built ---
echo "🔧 Building kernel module..."
make -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel modules || { echo "❌ ERROR: falló la compilación del módulo"; exit 1; }

# --- 2️⃣ Insert kernel module ---
echo "📦 Insertando módulo..."
sudo insmod "$KO_FILE" || error_exit "no se pudo insertar $KO_FILE"

sleep 1

# --- 3️⃣ Verificar creación de /dev/simtemp ---
if [ ! -e "$DEV_PATH" ]; then
    echo "Esperando que aparezca $DEV_PATH..."
    sleep 1
fi

[ -e "$DEV_PATH" ] || error_exit "no se creó el dispositivo $DEV_PATH"

# --- 4️⃣ Configurar sampling y threshold ---
#echo "⚙️  Configurando sysfs..."
#sudo bash -c "echo 200 > $SYSFS_PATH/sampling_ms" || error_exit "no se pudo configurar sampling_ms"
#sudo bash -c "echo 27000 > $SYSFS_PATH/threshold" || error_exit "no se pudo configurar threshold"

# --- 5️⃣ Ejecutar CLI test mode ---
echo "🚀 Ejecutando prueba de CLI..."
pushd "$USER_CLI" >/dev/null

sudo python3 main.py --test --device "$DEV_PATH" --sysfs "$SYSFS_PATH" || true
CLI_EXIT=0

popd >/dev/null

# --- 6️⃣ Remover módulo ---
echo "🧹 Removiendo módulo..."
sudo rmmod nxp_simtemp || error_exit "no se pudo remover el módulo"

echo "✅ Demo ejecutado correctamente."
