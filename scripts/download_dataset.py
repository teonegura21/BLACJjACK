#!/usr/bin/env python3
"""
Download and prepare Roboflow playing card detection dataset for YOLOv11 training
"""

import os
import sys
import argparse
from pathlib import Path

def download_roboflow_dataset(api_key: str, workspace: str, project: str, version: int, format: str = "yolov11"):
    """
    Download dataset from Roboflow using their Python SDK

    Args:
        api_key: Roboflow API key
        workspace: Workspace name (e.g., "augmented-startups")
        project: Project name (e.g., "playing-cards-ow27d")
        version: Dataset version number
        format: Export format (default: "yolov11")
    """
    try:
        from roboflow import Roboflow
    except ImportError:
        print("Error: roboflow package not installed")
        print("Install with: pip install roboflow")
        sys.exit(1)

    print(f"Connecting to Roboflow...")
    rf = Roboflow(api_key=api_key)

    print(f"Loading project: {workspace}/{project}")
    project = rf.workspace(workspace).project(project)

    print(f"Downloading dataset version {version} in {format} format...")
    dataset = project.version(version).download(format)

    print(f"\nDataset downloaded successfully to: {dataset.location}")
    print(f"  - Train images: {dataset.location}/train/images")
    print(f"  - Valid images: {dataset.location}/valid/images")
    print(f"  - Test images: {dataset.location}/test/images")
    print(f"  - Data YAML: {dataset.location}/data.yaml")

    return dataset.location


def setup_directory_structure(base_dir: str = "."):
    """Create necessary directory structure for training"""
    base_path = Path(base_dir)

    dirs = [
        "data/datasets",
        "data/train",
        "models/checkpoints",
        "models/onnx",
        "models/tensorrt",
        "logs"
    ]

    for dir_path in dirs:
        full_path = base_path / dir_path
        full_path.mkdir(parents=True, exist_ok=True)
        print(f"Created directory: {full_path}")


def create_training_yaml(dataset_path: str, output_path: str = "data/playing_cards.yaml"):
    """Create or update training YAML configuration"""
    import yaml

    config = {
        "path": dataset_path,
        "train": "train/images",
        "val": "valid/images",
        "test": "test/images",
        "nc": 52,  # Number of classes (52 cards)
        "names": [
            # Hearts
            "2H", "3H", "4H", "5H", "6H", "7H", "8H", "9H", "10H", "JH", "QH", "KH", "AH",
            # Diamonds
            "2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "10D", "JD", "QD", "KD", "AD",
            # Clubs
            "2C", "3C", "4C", "5C", "6C", "7C", "8C", "9C", "10C", "JC", "QC", "KC", "AC",
            # Spades
            "2S", "3S", "4S", "5S", "6S", "7S", "8S", "9S", "10S", "JS", "QS", "KS", "AS"
        ]
    }

    with open(output_path, 'w') as f:
        yaml.dump(config, f, default_flow_style=False)

    print(f"Training YAML created: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Download Roboflow playing cards dataset")
    parser.add_argument("--api-key", required=True, help="Roboflow API key")
    parser.add_argument("--workspace", default="augmented-startups",
                       help="Roboflow workspace name")
    parser.add_argument("--project", default="playing-cards-ow27d",
                       help="Roboflow project name")
    parser.add_argument("--version", type=int, default=1,
                       help="Dataset version")
    parser.add_argument("--format", default="yolov11",
                       help="Export format (yolov11, yolov8, etc)")
    parser.add_argument("--output-dir", default="./data/datasets",
                       help="Output directory for dataset")

    args = parser.parse_args()

    print("=" * 60)
    print("Roboflow Dataset Downloader for Blackjack AI Vision")
    print("=" * 60)
    print()

    # Setup directory structure
    setup_directory_structure()

    # Download dataset
    dataset_location = download_roboflow_dataset(
        api_key=args.api_key,
        workspace=args.workspace,
        project=args.project,
        version=args.version,
        format=args.format
    )

    # Create training YAML
    create_training_yaml(dataset_location)

    print()
    print("=" * 60)
    print("Setup Complete!")
    print("=" * 60)
    print()
    print("Next steps:")
    print("  1. Review the dataset in:", dataset_location)
    print("  2. Run training script: python scripts/train_yolov11.py")
    print("  3. Convert trained model to TensorRT")
    print()


if __name__ == "__main__":
    main()
