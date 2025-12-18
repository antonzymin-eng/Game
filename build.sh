#!/bin/bash
# Build script for Mechanica Imperii with Codespace-friendly resource limits
# This prevents browser tab freezing/reloading during compilation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect number of CPU cores
NPROC=$(nproc)
echo -e "${GREEN}Detected ${NPROC} CPU cores${NC}"

# Limit parallel jobs to prevent Codespace stalls
# Use 25-50% of available cores to keep browser responsive
if [ "$NPROC" -ge 12 ]; then
    # For high-core systems (Codespaces), use conservative limit
    JOBS=4
elif [ "$NPROC" -ge 8 ]; then
    JOBS=3
elif [ "$NPROC" -ge 4 ]; then
    JOBS=2
else
    JOBS=1
fi

# Allow override via environment variable
if [ -n "$BUILD_JOBS" ]; then
    JOBS=$BUILD_JOBS
    echo -e "${YELLOW}Using BUILD_JOBS override: ${JOBS}${NC}"
else
    echo -e "${GREEN}Using ${JOBS} parallel jobs (set BUILD_JOBS to override)${NC}"
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo -e "${GREEN}Creating build directory...${NC}"
    mkdir build
fi

cd build

# Configure with CMake
echo -e "${GREEN}Configuring with CMake...${NC}"
cmake .. "$@"

# Build with limited parallelism
echo -e "${GREEN}Building with ${JOBS} parallel jobs...${NC}"
cmake --build . -j ${JOBS}

echo -e "${GREEN}Build complete!${NC}"
