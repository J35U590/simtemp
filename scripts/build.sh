#!/bin/bash
set -e  # salir al primer error

# --- Colores para legibilidad ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No color

# --- Función auxiliar ---
fail() {
    echo -e "${RED}Error:${NC} $1" >&2
    exit 1
}

# --- Detectar kernel headers ---
KVER=$(uname -r)
KDIR="/lib/modules/$KVER/build"

echo -e "${YELLOW}[*] Verificando headers del kernel...${NC}"
if [ ! -d "$KDIR" ]; then
    fail "No se encontraron headers en $KDIR. Instálalos con:
    sudo apt install linux-headers-$(uname -r)"
fi
echo -e "${GREEN}[OK] Headers encontrados en $KDIR${NC}"

# --- Compilar módulo ---
echo -e "${YELLOW}[*] Compilando módulo del kernel...${NC}"
cd ~/kernel-modules/simtemp/kernel
make clean -C "$KDIR" M=$(pwd) modules || fail "Falló clean"
make -C "$KDIR" M=$(pwd) modules || fail "Falló la compilación del módulo"
echo -e "${GREEN}[OK] Módulo compilado correctamente${NC}"

# --- Compilar programas de usuario ---

echo -e "${YELLOW}[*] Compilando programa de usuario...${NC}"
cd ~/kernel-modules/simtemp/kernel
#poll_read.c
gcc -Wall -O2 poll_read.c -o poll_read || fail "Falló compilación de poll_read.c"
echo -e "${GREEN}[OK] Programa de usuario poll_read.c compilado${NC}"

#blocking_read.c
gcc -Wall -O2 blocking_read.c -o blocking_read || fail "Falló compilación de blocking_read.c"
echo -e "${GREEN}[OK] Programa de usuario blocking_read.c compilado${NC}"

#nonblocking_read.c
gcc -Wall -O2 nonblocking_read.c -o nonblocking_read || fail "Falló compilación de nonblocking_read.c"
echo -e "${GREEN}[OK] Programa de usuario nonblocking_read.c compilado${NC}"

#main.py
cd ~/kernel-modules/simtemp/user/cli
chmod +x main.py || fail "Falló compilación de main.py"
echo -e "${GREEN}[OK] Programa de usuario main.py compilado${NC}"

echo -e "${GREEN}✅ Build completado exitosamente.${NC}"

