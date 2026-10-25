"""Evaluation-only reproduction for UnifiedEEGNet + RA + 5-step IM-TTA.

No model training is performed. For each held-out subject, this script:
1. loads local BCI IV-2a GDF/MAT files through dataset.py,
2. recreates the exact LOSO subject-wise RA and source-pool normalization,
3. loads the target-specific validation-selected source checkpoint,
4. measures RA-only accuracy,
5. applies label-free BN-affine IM-TTA on target T+E trials,
6. saves subject-wise metrics and the 9-subject mean.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import sys
import time

import numpy as np
import pandas as pd
import torch
from torch.utils.data import DataLoader


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description="Evaluate saved UnifiedEEGNet LOSO checkpoints")
    parser.add_argument("--project_root", type=str, default="", help="Folder containing data/BCI_IV_2a")
    parser.add_argument("--code_dir", type=str, default="", help="Folder containing model.py, dataset.py, train.py")
    parser.add_argument("--checkpoint_dir", type=str, default="", help="Folder containing 9 .pth files")
    parser.add_argument("--output_dir", type=str, default="", help="Evaluation output folder")
    parser.add_argument("--batch_size", type=int, default=64)
    parser.add_argument("--tta_steps", type=int, default=5)
    parser.add_argument("--tta_lr", type=float, default=1e-3)
    parser.add_argument("--tta_div", type=float, default=1.0)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--device", choices=["auto", "cpu", "cuda"], default="auto")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    package_root = script_dir.parent if script_dir.name == "code" else script_dir
    project_root = Path(args.project_root).resolve() if args.project_root else package_root
    code_dir = Path(args.code_dir).resolve() if args.code_dir else script_dir
    checkpoint_dir = Path(args.checkpoint_dir).resolve() if args.checkpoint_dir else package_root / "checkpoints"
    output_dir = Path(args.output_dir).resolve() if args.output_dir else package_root / "evaluation_outputs"
    output_dir.mkdir(parents=True, exist_ok=True)

    sys.path.insert(0, str(code_dir))
    os.environ["LOCAL_BCI_PROJECT_ROOT"] = str(project_root)
    os.environ["PYTHONUTF8"] = "1"
    os.environ["PYTHONIOENCODING"] = "utf-8"

    from model import UnifiedEEGNet
    from dataset import load_dataset, prepare_loso_data, BCIDataset
    from train import evaluate, tent_adapt

    if args.device == "auto":
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    else:
        device = torch.device(args.device)
    if device.type == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but torch.cuda.is_available() is False")

    torch.manual_seed(args.seed)
    np.random.seed(args.seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(args.seed)

    started = time.time()
    print("Device:", device)
    if device.type == "cuda":
        print("GPU:", torch.cuda.get_device_name(0))
    print("Project root:", project_root)
    print("Code dir:", code_dir)
    print("Checkpoint dir:", checkpoint_dir)

    X, y, meta, n_classes, n_channels = load_dataset("iv2a", fmin=0.5, fmax=100.0)
    if X.shape != (5184, 22, 1000):
        raise ValueError(f"Unexpected full dataset shape: {X.shape}")

    rows = []
    expected_targets = [f"A{i:02d}" for i in range(1, 10)]

    for subject in range(1, 10):
        target = f"A{subject:02d}"
        ckpt_path = checkpoint_dir / f"loso_target_{target}_best.pth"
        if not ckpt_path.exists():
            raise FileNotFoundError(ckpt_path)

        payload = torch.load(ckpt_path, map_location="cpu", weights_only=False)
        meta_ckpt = payload.get("checkpoint_metadata", {})
        if meta_ckpt.get("target_subject") != target:
            raise ValueError(f"Checkpoint target mismatch: {ckpt_path.name} -> {meta_ckpt.get('target_subject')}")

        # Recreate the exact transformation used during training. Source labels are used
        # only for the source train/validation split; target labels are returned solely
        # for final metric calculation and are not passed to RA or IM-TTA loss.
        X_tr, X_va, X_te, y_tr, y_va, y_te = prepare_loso_data(
            X, y, meta, subject, val_frac=0.1, seed=args.seed, align="ra"
        )
        if X_te.shape != (576, 22, 1000):
            raise ValueError(f"{target}: unexpected target shape {X_te.shape}")

        test_ds = BCIDataset(X_te, y_te, augment=False, use_sr=False)
        test_loader = DataLoader(test_ds, batch_size=args.batch_size, shuffle=False, num_workers=0)

        model = UnifiedEEGNet(n_classes=n_classes, n_channels=n_channels).to(device)
        model.load_state_dict(payload["model_state_dict"], strict=True)

        ra_acc = evaluate(model, test_loader, device)
        adapted = tent_adapt(
            model, test_loader, device,
            steps=args.tta_steps, lr=args.tta_lr, div_weight=args.tta_div,
        )
        tta_acc = evaluate(adapted, test_loader, device)

        row = {
            "target_subject": target,
            "ra_only_accuracy": float(ra_acc),
            "ra_imtta_accuracy": float(tta_acc),
            "tta_gain": float(tta_acc - ra_acc),
            "best_epoch": int(payload["best_epoch"]),
            "best_validation_accuracy": float(payload["best_validation_accuracy"]),
            "checkpoint_file": ckpt_path.name,
            "checkpoint_sha256": sha256(ckpt_path),
        }
        rows.append(row)
        pd.DataFrame(rows).to_csv(output_dir / "checkpoint_evaluation_subjects.csv", index=False, encoding="utf-8-sig")
        print(f"{target}: RA={ra_acc*100:.2f}% | RA+IM-TTA={tta_acc*100:.2f}% | gain={(tta_acc-ra_acc)*100:+.2f}%p")

    df = pd.DataFrame(rows)
    mean_ra = float(df["ra_only_accuracy"].mean())
    mean_tta = float(df["ra_imtta_accuracy"].mean())
    std_tta_sample = float(df["ra_imtta_accuracy"].std(ddof=1))
    summary = {
        "success": True,
        "evaluation_mode": "offline_transductive_target_label_free_loso",
        "checkpoint_count": len(rows),
        "mean_ra_only_accuracy": mean_ra,
        "mean_ra_imtta_accuracy": mean_tta,
        "std_ra_imtta_accuracy_sample": std_tta_sample,
        "mean_tta_gain": float(df["tta_gain"].mean()),
        "submission_threshold": 0.69,
        "passes_69_percent": bool(mean_tta >= 0.69),
        "tta_steps": args.tta_steps,
        "tta_lr": args.tta_lr,
        "tta_diversity_weight": args.tta_div,
        "seed": args.seed,
        "target_trials_per_subject": 576,
        "target_labels_used_in_alignment_or_tta": False,
        "target_labels_used_for_final_metric_only": True,
        "elapsed_seconds": time.time() - started,
        "python": sys.version,
        "torch": torch.__version__,
        "device": str(device),
        "gpu": torch.cuda.get_device_name(0) if device.type == "cuda" else None,
    }
    (output_dir / "checkpoint_evaluation_summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8"
    )

    print("\n" + "=" * 90)
    print(df[["target_subject", "ra_only_accuracy", "ra_imtta_accuracy", "tta_gain"]].to_string(index=False))
    print("=" * 90)
    print(f"Mean RA-only Accuracy: {mean_ra*100:.2f}%")
    print(f"Mean RA+IM-TTA Accuracy: {mean_tta*100:.2f}%")
    print(f"Sample SD: {std_tta_sample*100:.2f}%")
    print(f"69% threshold: {'PASS' if mean_tta >= 0.69 else 'FAIL'}")
    print("Saved:", output_dir)


if __name__ == "__main__":
    main()
