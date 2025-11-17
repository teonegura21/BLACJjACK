#!/usr/bin/env python3
"""
Train YOLOv11X model for playing card detection
Optimized for RTX 4060 with 2K resolution support
"""

import argparse
from pathlib import Path
import torch

def check_requirements():
    """Check if all required packages are installed"""
    try:
        import ultralytics
        from ultralytics import YOLO
        print(f"✓ Ultralytics YOLO installed (version: {ultralytics.__version__})")
    except ImportError:
        print("✗ Ultralytics not installed")
        print("  Install with: pip install ultralytics")
        return False

    # Check CUDA availability
    if torch.cuda.is_available():
        print(f"✓ CUDA available (version: {torch.version.cuda})")
        print(f"  Device: {torch.cuda.get_device_name(0)}")
        print(f"  VRAM: {torch.cuda.get_device_properties(0).total_memory / 1e9:.2f} GB")
    else:
        print("✗ CUDA not available - training will be very slow!")
        return False

    return True


def train_yolov11x(
    data_yaml: str,
    epochs: int = 300,
    imgsz: int = 1280,
    batch: int = 8,
    device: int = 0,
    project: str = "models/train",
    name: str = "yolov11x_cards",
    resume: bool = False
):
    """
    Train YOLOv11X model for card detection

    Args:
        data_yaml: Path to dataset YAML file
        epochs: Number of training epochs (default: 300)
        imgsz: Input image size (default: 1280 for 2K support)
        batch: Batch size (default: 8 for RTX 4060)
        device: GPU device ID (default: 0)
        project: Project directory for outputs
        name: Run name
        resume: Resume from last checkpoint
    """
    from ultralytics import YOLO

    print("=" * 80)
    print("YOLOv11X Training for Playing Card Detection")
    print("=" * 80)
    print()

    # Load model
    if resume:
        print(f"Resuming training from: {project}/{name}/weights/last.pt")
        model = YOLO(f"{project}/{name}/weights/last.pt")
    else:
        print("Loading YOLOv11X pretrained weights...")
        model = YOLO('yolov11x.pt')

    print()
    print("Training Configuration:")
    print(f"  Data YAML: {data_yaml}")
    print(f"  Epochs: {epochs}")
    print(f"  Image Size: {imgsz}x{imgsz}")
    print(f"  Batch Size: {batch}")
    print(f"  Device: cuda:{device}")
    print(f"  Project: {project}")
    print(f"  Name: {name}")
    print()

    # Training parameters optimized for RTX 4060
    results = model.train(
        data=data_yaml,
        epochs=epochs,
        imgsz=imgsz,
        batch=batch,
        device=device,
        project=project,
        name=name,
        patience=50,
        save=True,
        save_period=10,  # Save checkpoint every 10 epochs

        # Optimizer settings
        optimizer='AdamW',
        lr0=0.001,      # Initial learning rate
        lrf=0.01,       # Final learning rate (lr0 * lrf)
        momentum=0.937,
        weight_decay=0.0005,
        warmup_epochs=5,
        warmup_momentum=0.8,
        warmup_bias_lr=0.1,

        # Augmentation for robustness
        hsv_h=0.015,    # Hue augmentation
        hsv_s=0.7,      # Saturation augmentation
        hsv_v=0.4,      # Value augmentation
        degrees=15.0,   # Rotation (+/- deg)
        translate=0.1,  # Translation (+/- fraction)
        scale=0.5,      # Scale (+/- gain)
        shear=0.0,      # Shear (+/- deg)
        perspective=0.0,# Perspective (+/- fraction)
        flipud=0.0,     # Flip up-down probability
        fliplr=0.5,     # Flip left-right probability
        mosaic=1.0,     # Mosaic augmentation probability
        mixup=0.1,      # Mixup augmentation probability
        copy_paste=0.0, # Copy-paste augmentation probability

        # Hardware optimization
        amp=True,       # Automatic Mixed Precision (FP16 training)
        workers=8,      # DataLoader workers
        cache=True,     # Cache images for faster training

        # Validation
        val=True,
        plots=True,     # Save training plots

        # Other
        verbose=True,
        seed=0,
        deterministic=True,
        single_cls=False,
        rect=False,     # Rectangular training
        cos_lr=False,   # Cosine learning rate scheduler
        close_mosaic=10 # Disable mosaic last N epochs
    )

    print()
    print("=" * 80)
    print("Training Complete!")
    print("=" * 80)
    print()
    print(f"Best weights: {project}/{name}/weights/best.pt")
    print(f"Last weights: {project}/{name}/weights/last.pt")
    print()
    print("Validation Results:")
    print(f"  mAP@0.5: {results.results_dict.get('metrics/mAP50(B)', 'N/A')}")
    print(f"  mAP@0.5:0.95: {results.results_dict.get('metrics/mAP50-95(B)', 'N/A')}")
    print()

    return results


def export_to_onnx(
    weights_path: str,
    imgsz: int = 1280,
    output_dir: str = "models/onnx"
):
    """Export trained model to ONNX format"""
    from ultralytics import YOLO

    print("=" * 80)
    print("Exporting to ONNX")
    print("=" * 80)

    model = YOLO(weights_path)

    print(f"Loading weights from: {weights_path}")
    print(f"Image size: {imgsz}x{imgsz}")
    print(f"Output directory: {output_dir}")
    print()

    # Create output directory
    Path(output_dir).mkdir(parents=True, exist_ok=True)

    # Export to ONNX
    output_path = model.export(
        format='onnx',
        imgsz=imgsz,
        half=True,      # FP16 for TensorRT
        simplify=True,  # Simplify ONNX graph
        opset=17,       # ONNX opset version
        dynamic=False,  # Fixed input size for TensorRT optimization
        verbose=True
    )

    print()
    print(f"✓ ONNX model exported: {output_path}")
    print()
    print("Next steps:")
    print(f"  Convert to TensorRT: trtexec --onnx={output_path} \\")
    print("    --saveEngine=models/yolov11x_card_detector.trt \\")
    print("    --fp16 --workspace=4096 --useCudaGraph")
    print()

    return output_path


def main():
    parser = argparse.ArgumentParser(description="Train YOLOv11X for card detection")
    parser.add_argument("--data", default="data/playing_cards.yaml",
                       help="Path to data YAML file")
    parser.add_argument("--epochs", type=int, default=300,
                       help="Number of training epochs")
    parser.add_argument("--imgsz", type=int, default=1280,
                       help="Input image size")
    parser.add_argument("--batch", type=int, default=8,
                       help="Batch size")
    parser.add_argument("--device", type=int, default=0,
                       help="CUDA device ID")
    parser.add_argument("--project", default="models/train",
                       help="Project directory")
    parser.add_argument("--name", default="yolov11x_cards",
                       help="Run name")
    parser.add_argument("--resume", action="store_true",
                       help="Resume from last checkpoint")
    parser.add_argument("--export", action="store_true",
                       help="Export to ONNX after training")

    args = parser.parse_args()

    # Check requirements
    if not check_requirements():
        print("\nPlease install missing requirements before continuing.")
        return 1

    print()

    # Train model
    results = train_yolov11x(
        data_yaml=args.data,
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        project=args.project,
        name=args.name,
        resume=args.resume
    )

    # Export to ONNX if requested
    if args.export:
        best_weights = f"{args.project}/{args.name}/weights/best.pt"
        export_to_onnx(best_weights, imgsz=args.imgsz)

    return 0


if __name__ == "__main__":
    exit(main())
