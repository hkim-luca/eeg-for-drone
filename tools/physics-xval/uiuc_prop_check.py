"""Open-data validation of the drone airframe presets (oracle layer 4).

Judges the kT/kQ coefficients of every built-in preset in
DronePhysicsPresets.cpp against the UIUC Propeller Data Site
(https://m-selig.ae.illinois.edu/props/) - wind-tunnel static thrust/power
measurements. Dimensionless CT/CP from a geometrically similar propeller are
rescaled to each preset's rotor diameter:

    T = CT rho n^2 D^4,  w = 2 pi n  ->  kT = CT rho D^4 / (4 pi^2)
    Q = CQ rho n^2 D^5,  CQ = CP/(2 pi)  ->  kQ = CP rho D^5 / (8 pi^3)

Two judges per preset: the kT/kQ ratio against the open data (pass inside
[1/3, 3]) and the momentum-theory figure of merit implied by the preset's own
coefficients (physical rotors sit inside [0.3, 0.85]).

Usage: python uiuc_prop_check.py   (downloads are cached in ./data)
"""

from __future__ import annotations

import math
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Final

BASE_URL: Final[str] = "https://m-selig.ae.illinois.edu/props/"
DATA_DIR: Final[Path] = Path(__file__).resolve().parent / "data"
SEA_LEVEL_AIR_DENSITY: Final[float] = 1.225
GRAVITY: Final[float] = 9.80665
RATIO_BOUNDS: Final[tuple[float, float]] = (1.0 / 3.0, 3.0)
FM_BOUNDS: Final[tuple[float, float]] = (0.3, 0.85)


@dataclass
class PresetSpec:
    """One built-in preset from DronePhysicsPresets.cpp plus its matched
    UIUC propellers (nearest diameter; empty = figure-of-merit check only)."""

    name: str
    mass_kg: float
    motor_count: int
    thrust_coeff: float
    torque_coeff: float
    rotor_radius_m: float
    motor_max_rad_s: float
    prop_files: tuple[str, ...]


PRESETS: Final[tuple[PresetSpec, ...]] = (
    PresetSpec("MICRO 250 g", 0.249, 4, 7.04e-7, 5.88e-9, 0.06, 1600.0,
               ("volume-2/data/gwsdd_5x3_static_0323rd.txt",)),
    PresetSpec("FPV RACER 600 g", 0.6, 4, 1.01e-6, 1.50e-8, 0.0635, 2700.0,
               ("volume-2/data/da4002_5x3.75_static_1121md.txt",)),
    PresetSpec("STANDARD 1.2 kg", 1.2, 4, 1.21e-5, 1.89e-7, 0.12, 780.0,
               ("volume-1/data/apcsf_9x4.7_static_kt1032.txt",
                "volume-1/data/apcsf_10x4.7_static_kt0835.txt")),
    PresetSpec("CINEMA 2.5 kg", 2.5, 4, 3.81e-5, 7.11e-7, 0.17, 670.0,
               ("volume-3/data/ancf_13x65_static_0570od.txt",)),
    PresetSpec("INDUSTRIAL 6.5 kg", 6.5, 6, 2.34e-4, 6.99e-6, 0.27, 365.0,
               ("volume-3/data/ancf_16x8_static_0997od.txt",)),  # nearest: 16 in vs 21 in
    PresetSpec("HEAVY LIFT 25 kg", 25.0, 8, 6.8e-4, 2e-5, 0.5, 370.0, ()),
)


def fetch_static_file(relative_path: str) -> Path:
    """Downloads one UIUC data file into the local cache; reuses it offline."""
    local = DATA_DIR / Path(relative_path).name
    if not local.exists():
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        with urllib.request.urlopen(BASE_URL + relative_path, timeout=30) as response:
            local.write_bytes(response.read())
    return local


def mean_ct_cp(paths: tuple[str, ...]) -> tuple[float, float]:
    """Mean static CT and CP over every RPM row of the given data files."""
    ct_values: list[float] = []
    cp_values: list[float] = []
    for relative_path in paths:
        for line in fetch_static_file(relative_path).read_text().splitlines():
            fields = line.split()
            if len(fields) == 3 and not fields[0].startswith("RPM"):
                ct_values.append(float(fields[1]))
                cp_values.append(float(fields[2]))
    if not ct_values:
        raise ValueError(f"no data rows parsed from {paths}")
    return sum(ct_values) / len(ct_values), sum(cp_values) / len(cp_values)


def in_bounds(value: float, bounds: tuple[float, float]) -> bool:
    return bounds[0] <= value <= bounds[1]


def main() -> int:
    print(f"{'preset':<20} {'kT ratio':>9} {'kQ ratio':>9} {'FM':>6} "
          f"{'hover krpm':>11} {'data krpm':>10}  verdict")
    all_passed = True
    for preset in PRESETS:
        diameter = 2.0 * preset.rotor_radius_m
        # figure of merit implied by the preset's own kT/kQ, no external data needed
        ct_implied = preset.thrust_coeff * 4.0 * math.pi**2 / (SEA_LEVEL_AIR_DENSITY * diameter**4)
        cp_implied = preset.torque_coeff * 8.0 * math.pi**3 / (SEA_LEVEL_AIR_DENSITY * diameter**5)
        figure_of_merit = ct_implied**1.5 / (math.sqrt(2.0) * cp_implied)
        hover_rad_s = math.sqrt(preset.mass_kg * GRAVITY / (preset.motor_count * preset.thrust_coeff))
        hover_krpm = hover_rad_s * 60.0 / (2.0 * math.pi) / 1000.0

        verdicts: list[str] = []
        if not in_bounds(figure_of_merit, FM_BOUNDS):
            verdicts.append(f"FM {figure_of_merit:.2f} outside {FM_BOUNDS}")

        if preset.prop_files:
            ct_data, cp_data = mean_ct_cp(preset.prop_files)
            kt_data = ct_data * SEA_LEVEL_AIR_DENSITY * diameter**4 / (4.0 * math.pi**2)
            kq_data = cp_data * SEA_LEVEL_AIR_DENSITY * diameter**5 / (8.0 * math.pi**3)
            kt_ratio = preset.thrust_coeff / kt_data
            kq_ratio = preset.torque_coeff / kq_data
            hover_data_krpm = (math.sqrt(preset.mass_kg * GRAVITY / (preset.motor_count * kt_data))
                               * 60.0 / (2.0 * math.pi) / 1000.0)
            if not in_bounds(kt_ratio, RATIO_BOUNDS):
                verdicts.append(f"kT {kt_ratio:.2f}x off open data")
            if not in_bounds(kq_ratio, RATIO_BOUNDS):
                verdicts.append(f"kQ {kq_ratio:.2f}x off open data")
            ratio_text = (f"{kt_ratio:9.2f} {kq_ratio:9.2f} {figure_of_merit:6.2f} "
                          f"{hover_krpm:11.1f} {hover_data_krpm:10.1f}")
        else:
            ratio_text = (f"{'-':>9} {'-':>9} {figure_of_merit:6.2f} "
                          f"{hover_krpm:11.1f} {'-':>10}")

        passed = not verdicts
        all_passed = all_passed and passed
        print(f"{preset.name:<20} {ratio_text}  {'OK' if passed else '; '.join(verdicts)}")

    print(f"\nopen-data validation {'PASSED' if all_passed else 'FAILED'}")
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
