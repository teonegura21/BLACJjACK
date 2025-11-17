# Blackjack AI Vision - Production Docker Image
# Based on NVIDIA CUDA 12.0 with TensorRT 8.6
FROM nvidia/cuda:12.0.1-cudnn8-runtime-ubuntu22.04

# Metadata
LABEL maintainer="Blackjack AI Vision"
LABEL version="2.5"
LABEL description="Real-time blackjack card detection and counting system"

# Environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_HOME=/usr/local/cuda
ENV LD_LIBRARY_PATH=/usr/local/cuda/lib64:/usr/local/lib:$LD_LIBRARY_PATH
ENV PATH=/usr/local/cuda/bin:$PATH

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-pip \
    libjson-c-dev \
    libgl1-mesa-glx \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxrender-dev \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Install TensorRT 8.6 (if not included in base image)
# Note: Adjust version based on your CUDA version
RUN apt-get update && apt-get install -y \
    libnvinfer8 \
    libnvinfer-plugin8 \
    libnvparsers8 \
    libnvonnxparsers8 \
    libnvinfer-dev \
    libnvinfer-plugin-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy application files
COPY build/bin/blackjack_ai_vision /app/
COPY config.json /app/
COPY models /app/models
COPY scripts /app/scripts

# Create necessary directories
RUN mkdir -p /app/data /app/logs

# Install Python dependencies for scripts
RUN pip3 install --no-cache-dir \
    numpy \
    opencv-python-headless \
    pyyaml

# Set permissions
RUN chmod +x /app/blackjack_ai_vision

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pgrep -x blackjack_ai_vision || exit 1

# Expose ports (if using web interface)
# EXPOSE 8080

# Default command
CMD ["/app/blackjack_ai_vision", "--config", "/app/config.json"]
