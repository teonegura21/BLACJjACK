#!/bin/bash
# Setup script for Blackjack AI Vision
# Automated environment setup for Ubuntu 22.04

set -e  # Exit on error

echo "========================================"
echo "Blackjack AI Vision - Setup Script"
echo "========================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo -e "${RED}Please do not run this script as root${NC}"
    exit 1
fi

echo -e "${GREEN}[1/7] Checking system requirements...${NC}"

# Check Ubuntu version
if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" != "ubuntu" ] || [ "${VERSION_ID}" != "22.04" ]; then
        echo -e "${YELLOW}Warning: This script is optimized for Ubuntu 22.04${NC}"
        echo -e "${YELLOW}Your system: $ID $VERSION_ID${NC}"
        read -p "Continue anyway? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
fi

# Check NVIDIA driver
if ! command -v nvidia-smi &> /dev/null; then
    echo -e "${RED}NVIDIA driver not found. Please install NVIDIA driver 545.84 or newer.${NC}"
    exit 1
fi

DRIVER_VERSION=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -n1)
echo -e "${GREEN}✓ NVIDIA Driver version: $DRIVER_VERSION${NC}"

echo
echo -e "${GREEN}[2/7] Installing system dependencies...${NC}"

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-pip \
    python3-venv \
    libjson-c-dev \
    libgl1-mesa-glx \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxrender-dev \
    libgomp1

echo -e "${GREEN}✓ System dependencies installed${NC}"

echo
echo -e "${GREEN}[3/7] Setting up Python environment...${NC}"

# Create virtual environment
if [ ! -d "venv" ]; then
    python3 -m venv venv
    echo -e "${GREEN}✓ Virtual environment created${NC}"
else
    echo -e "${YELLOW}Virtual environment already exists${NC}"
fi

# Activate virtual environment
source venv/bin/activate

# Upgrade pip
pip install --upgrade pip setuptools wheel

# Install Python dependencies
pip install -r requirements.txt

echo -e "${GREEN}✓ Python dependencies installed${NC}"

echo
echo -e "${GREEN}[4/7] Checking CUDA installation...${NC}"

if [ -d "/usr/local/cuda" ]; then
    CUDA_VERSION=$(nvcc --version | grep "release" | awk '{print $5}' | cut -d',' -f1)
    echo -e "${GREEN}✓ CUDA version: $CUDA_VERSION${NC}"
else
    echo -e "${YELLOW}Warning: CUDA not found at /usr/local/cuda${NC}"
    echo -e "${YELLOW}Please install CUDA Toolkit 12.0 or newer${NC}"
    echo "Download from: https://developer.nvidia.com/cuda-downloads"
fi

echo
echo -e "${GREEN}[5/7] Checking TensorRT installation...${NC}"

if [ -d "/usr/local/TensorRT" ] || [ -d "/usr/lib/x86_64-linux-gnu" ]; then
    if ldconfig -p | grep -q libnvinfer; then
        TRT_VERSION=$(dpkg -l | grep nvinfer | head -n1 | awk '{print $3}')
        echo -e "${GREEN}✓ TensorRT version: $TRT_VERSION${NC}"
    else
        echo -e "${YELLOW}TensorRT libraries not found${NC}"
    fi
else
    echo -e "${YELLOW}Warning: TensorRT not found${NC}"
    echo -e "${YELLOW}Please install TensorRT 8.6 or newer${NC}"
    echo "Download from: https://developer.nvidia.com/tensorrt"
fi

echo
echo -e "${GREEN}[6/7] Creating directory structure...${NC}"

# Create necessary directories
mkdir -p data/{datasets,sessions,train}
mkdir -p models/{checkpoints,onnx,tensorrt,train}
mkdir -p logs
mkdir -p scripts
mkdir -p build

echo -e "${GREEN}✓ Directory structure created${NC}"

echo
echo -e "${GREEN}[7/7] Verifying setup...${NC}"

# Check Python packages
python3 -c "import torch; print(f'PyTorch: {torch.__version__}')"
python3 -c "import ultralytics; print(f'Ultralytics: {ultralytics.__version__}')"
python3 -c "import cv2; print(f'OpenCV: {cv2.__version__}')"

# Check CUDA availability in PyTorch
if python3 -c "import torch; assert torch.cuda.is_available()" 2>/dev/null; then
    GPU_NAME=$(python3 -c "import torch; print(torch.cuda.get_device_name(0))")
    VRAM=$(python3 -c "import torch; print(f'{torch.cuda.get_device_properties(0).total_memory / 1e9:.2f} GB')")
    echo -e "${GREEN}✓ PyTorch CUDA available${NC}"
    echo -e "${GREEN}  GPU: $GPU_NAME${NC}"
    echo -e "${GREEN}  VRAM: $VRAM${NC}"
else
    echo -e "${RED}✗ PyTorch CUDA not available${NC}"
    echo -e "${YELLOW}Training will be very slow without CUDA!${NC}"
fi

echo
echo "========================================"
echo -e "${GREEN}Setup Complete!${NC}"
echo "========================================"
echo
echo "Next steps:"
echo "  1. Activate virtual environment: source venv/bin/activate"
echo "  2. Download dataset: python scripts/download_dataset.py --api-key YOUR_KEY"
echo "  3. Train model: python scripts/train_yolov11.py"
echo "  4. Build application: cd build && cmake .. && make -j\$(nproc)"
echo
echo "For quick start guide, see: QUICKSTART.md"
echo "For full deployment plan, see: DEPLOYMENT_UPGRADE_PLAN.md"
echo
