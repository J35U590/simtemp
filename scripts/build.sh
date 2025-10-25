#!/bin/bash
set -e  # leave at the first mistake

# --- Colors for readability ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No color

# --- Auxiliar function ---
fail() {
    echo -e "${RED}Error:${NC} $1" >&2
    exit 1
}

# --- Detect kernel headers ---
KVER=$(uname -r)
KDIR="/lib/modules/$KVER/build"

echo -e "${YELLOW}[*] Checking kernel headers...${NC}"
if [ ! -d "$KDIR" ]; then
    fail "No headers were found in $KDIR. Install them with:
    sudo apt install linux-headers-$(uname -r)"
fi
echo -e "${GREEN}[OK] Headers found in $KDIR${NC}"

# --- Compile module ---
echo -e "${YELLOW}[*] Compiling kernel module...${NC}"
cd ~/kernel-modules/simtemp/kernel
make clean -C "$KDIR" M=$(pwd) modules || fail "Clean failed"
make -C "$KDIR" M=$(pwd) modules || fail "Module compilation failed"
echo -e "${GREEN}[OK] Module compiled successfully${NC}"

# --- Compiling user programs ---

echo -e "${YELLOW}[*] Compiling user program...${NC}"
cd ~/kernel-modules/simtemp/kernel
#poll_read.c
gcc -Wall -O2 poll_read.c -o poll_read || fail "poll_read.c compilation failed"
echo -e "${GREEN}[OK] poll_read.c compiled${NC}"

#blocking_read.c
gcc -Wall -O2 blocking_read.c -o blocking_read || fail "blocking_read.c compilation failed"
echo -e "${GREEN}[OK] blocking_read.c compiled${NC}"

#nonblocking_read.c
gcc -Wall -O2 nonblocking_read.c -o nonblocking_read || fail "nonblocking_read.c compilation failed"
echo -e "${GREEN}[OK] nonblocking_read.c compiled${NC}"

#main.py
cd ~/kernel-modules/simtemp/user/cli
chmod +x main.py || fail "main.py compilation failed"
echo -e "${GREEN}[OK] main.py compiled${NC}"

echo -e "${GREEN}✅ Build completed successfully.${NC}"

