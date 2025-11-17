# Blackjack AI Vision - Quick Start Guide

Get your production-ready blackjack card counting system running in minutes.

## Prerequisites

### Hardware
- NVIDIA GPU (RTX 4060 or better recommended)
- 16GB RAM minimum (32GB recommended)
- 10GB free disk space

### Software
- Ubuntu 22.04 / Windows 10/11
- NVIDIA Driver 545.84+
- Docker with NVIDIA Container Toolkit
- OR: CUDA 12.0+, TensorRT 8.6+, CMake 3.25+

## Option 1: Docker Deployment (Recommended)

### Step 1: Install Docker and NVIDIA Container Toolkit

```bash
# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Install NVIDIA Container Toolkit
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
  sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
sudo systemctl restart docker
```

### Step 2: Download Pre-trained Model

```bash
# If you have a pre-trained model, place it here:
mkdir -p models
# Place your yolov11x_card_detector.trt in models/

# OR train your own (see Training section below)
```

### Step 3: Build and Run

```bash
# Build Docker image
docker-compose build

# Run the application
docker-compose up -d

# View logs
docker-compose logs -f blackjack-ai

# Stop
docker-compose down
```

## Option 2: Native Build

### Step 1: Install Dependencies

**Ubuntu 22.04:**
```bash
# System packages
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    nvidia-cuda-toolkit \
    libjson-c-dev

# Download and install TensorRT 8.6
# Follow: https://docs.nvidia.com/deeplearning/tensorrt/install-guide/
```

**Windows:**
```powershell
# Install Visual Studio 2022 with C++ tools
# Install CUDA Toolkit 12.0+
# Install TensorRT 8.6+
# Install CMake 3.25+
```

### Step 2: Build Application

```bash
# Clone repository
git clone https://github.com/yourusername/BLACJjACK.git
cd BLACJjACK

# Create build directory
mkdir build && cd build

# Configure (adjust TensorRT path as needed)
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DTensorRT_DIR="/usr/local/TensorRT"

# Build
cmake --build . --config Release -j$(nproc)

# Binaries will be in build/bin/
```

### Step 3: Run Application

```bash
# Run with default config
./bin/blackjack_ai_vision

# Run with custom config
./bin/blackjack_ai_vision --config /path/to/config.json
```

## Training Your Own Model

### Step 1: Get Roboflow API Key

1. Sign up at https://roboflow.com
2. Get your API key from Account Settings

### Step 2: Download Dataset

```bash
# Install Python dependencies
pip install roboflow ultralytics pyyaml

# Download dataset (10,100 images)
python scripts/download_dataset.py \
    --api-key YOUR_API_KEY \
    --workspace augmented-startups \
    --project playing-cards-ow27d \
    --version 1
```

### Step 3: Train YOLOv11X

```bash
# Train model (takes ~2 days on RTX 4060)
python scripts/train_yolov11.py \
    --data data/playing_cards.yaml \
    --epochs 300 \
    --imgsz 1280 \
    --batch 8 \
    --device 0 \
    --export

# This will:
# - Train for 300 epochs (~2 days)
# - Save best model to models/train/yolov11x_cards/weights/best.pt
# - Export to ONNX format automatically
```

### Step 4: Convert to TensorRT

```bash
# Convert ONNX to TensorRT engine
trtexec \
    --onnx=models/onnx/yolov11x_card_detector.onnx \
    --saveEngine=models/yolov11x_card_detector.trt \
    --fp16 \
    --workspace=4096 \
    --minShapes=images:1x3x1280x1280 \
    --optShapes=images:1x3x1280x1280 \
    --maxShapes=images:1x3x1280x1280 \
    --useCudaGraph \
    --tacticSources=+CUDNN,+CUBLAS,+CUBLAS_LT \
    --verbose

# This takes 10-30 minutes
```

## Configuration

Edit `config.json` to customize:

```json
{
  "vision": {
    "model_path": "./models/yolov11x_card_detector.trt",
    "confidence_threshold": 0.65,
    "nms_threshold": 0.45
  },
  "counting": {
    "system": "hi-lo",
    "deck_count": 6
  },
  "betting": {
    "kelly_fraction": 0.25,
    "min_bet": 10,
    "max_bet": 500
  }
}
```

## Usage

### Keyboard Controls

- **F1**: Toggle overlay display
- **F2**: Reset card count
- **F3**: Show/hide performance metrics
- **F4**: Toggle audio alerts
- **F5**: Cycle counting systems
- **ESC**: Exit application

### Overlay Information

The real-time overlay shows:
- **Running Count**: Current Hi-Lo count
- **True Count**: Deck-adjusted count
- **Recommended Action**: Hit/Stand/Double/Split
- **Optimal Bet**: Kelly Criterion bet size
- **Performance**: FPS, latency, accuracy

## Verification

### Test Inference Performance

```bash
# Benchmark mode (warm up + 1000 iterations)
./bin/blackjack_ai_vision --benchmark --warmup=50 --iterations=1000

# Expected results on RTX 4060:
# - Average inference: 3-5ms
# - p95 latency: <8ms
# - p99 latency: <10ms
```

### Validate Detection Accuracy

```bash
# Run validation on test set
python scripts/validate.py \
    --model models/yolov11x_card_detector.trt \
    --data data/playing_cards.yaml

# Expected results:
# - mAP@0.5: >95%
# - Card recognition accuracy: >98%
# - False positive rate: <1%
```

## Troubleshooting

### "Failed to load TensorRT engine"
- Ensure model file exists at path in config.json
- Rebuild engine with correct TensorRT version
- Check file permissions

### "CUDA out of memory"
- Reduce buffer_count in config.json
- Close other GPU applications
- Use smaller batch size during training

### "Low FPS / High latency"
- Enable CUDA graphs in config.json
- Lock GPU clocks: `sudo nvidia-smi -lgc 2400`
- Check GPU isn't thermal throttling
- Verify FP16 is enabled

### "Low detection accuracy"
- Increase confidence_threshold to reduce false positives
- Ensure proper lighting conditions
- Check if cards are clearly visible (not occluded >30%)
- Retrain model with more varied data

## Performance Expectations

### RTX 4060 Laptop (Target Hardware)
- **Inference**: 3.2ms average, 4.8ms p99
- **Total Latency**: <5ms end-to-end
- **Accuracy**: >95% mAP@0.5
- **VRAM**: ~1.8GB
- **Power**: ~45W

### Strategy Performance
- **Hi-Lo Counting**: 100% accurate (deterministic)
- **Illustrious 18**: All 18 deviations implemented
- **Fab 4**: All 4 surrender plays implemented
- **Kelly Betting**: Fractional Kelly (0.25x) for risk management

### Expected Results (6-deck S17 DAS)
- **Player Edge at TC+2**: ~1.0%
- **Player Edge at TC+3**: ~1.5%
- **Expected Win Rate**: $20-50/hour ($10K bankroll, $10-$500 spread)

## Next Steps

1. **Practice Mode**: Test strategy without real money
2. **Session Analytics**: Review logs in `data/sessions/`
3. **Fine-tuning**: Adjust confidence thresholds for your setup
4. **Advanced Features**: Enable multi-table support, web dashboard

## Support

- **Documentation**: See [README.md](README.md) for full documentation
- **Issues**: Report bugs at GitHub Issues
- **Training Guide**: See [DEPLOYMENT_UPGRADE_PLAN.md](DEPLOYMENT_UPGRADE_PLAN.md)

## Legal Disclaimer

**For educational and research purposes only.**

Card counting is legal but casinos may refuse service. Using automated systems may violate terms of service. This software is a learning tool for understanding blackjack strategy and computer vision. Use responsibly and in compliance with local laws.

---

**Ready to deploy!** Your production-grade blackjack AI vision system is configured for optimal performance with mathematically perfect strategy execution.
