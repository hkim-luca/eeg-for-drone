"""Independent cross-validation of FDroneFlightModel (oracle layer 3).

Re-integrates the exact same 6-DOF ODE as DroneFlightModel.cpp with scipy's
DOP853 at 1e-12 tolerance and compares every sample of the C++ reference
trajectory written by DroneSim.Physics.RedTeam.CrossValidationDump
(DroneSim/Saved/RedTeam/xval_cpp.csv). Any equation-level disagreement between
the two implementations shows up as an error orders of magnitude above the
RK4 truncation error.

Usage: python xval_flight_model.py [path/to/xval_cpp.csv]
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Final

import numpy as np
import numpy.typing as npt
from scipy.integrate import solve_ivp

DEFAULT_CSV: Final[str] = str(
    Path(__file__).resolve().parents[2] / "DroneSim" / "Saved" / "RedTeam" / "xval_cpp.csv"
)
SEA_LEVEL_AIR_DENSITY: Final[float] = 1.225
SPIN_DIRECTION: Final[tuple[float, float, float, float]] = (+1.0, -1.0, +1.0, -1.0)

# agreement thresholds: RK4@1kHz truncation error is ~1e-9; an equation mismatch
# produces errors of 1e-1 or more, so these bounds have 5 decades of margin
TOL_POSITION_M: Final[float] = 1e-4
TOL_VELOCITY_MS: Final[float] = 1e-4
TOL_QUATERNION: Final[float] = 1e-5
TOL_ANGULAR_RATE: Final[float] = 1e-4
TOL_MOTOR_RADS: Final[float] = 1e-2


@dataclass
class ModelParams:
    mass_kg: float = 0.0
    arm_length_m: float = 0.0
    inertia_xx: float = 0.0
    inertia_yy: float = 0.0
    inertia_zz: float = 0.0
    motor_tau_s: float = 0.0
    thrust_coeff: float = 0.0
    torque_coeff: float = 0.0
    motor_max_rad_s: float = 0.0
    rotor_inertia: float = 0.0
    gravity: float = 0.0
    air_density: float = 0.0
    drag_linear: float = 0.0
    drag_quadratic: float = 0.0
    yaw_rad: float = 0.0
    hover_rad_s: float = 0.0


def load_reference(csv_path: Path) -> tuple[ModelParams, npt.NDArray[np.float64]]:
    """Reads the C++ dump: '#param,name,value' header lines plus one sample per row."""
    params = ModelParams()
    rows: list[list[float]] = []
    for line in csv_path.read_text().splitlines():
        if line.startswith("#param,"):
            _, name, value = line.split(",")
            setattr(params, name, float(value))
        elif line and not line.startswith(("t,", "#")):
            rows.append([float(field) for field in line.split(",")])
    if not rows:
        raise ValueError(f"no samples found in {csv_path}")
    return params, np.array(rows, dtype=np.float64)


def quat_mult(q: npt.NDArray[np.float64], p: npt.NDArray[np.float64]) -> npt.NDArray[np.float64]:
    """Hamilton product with UE component order (x, y, z, w); q*p applies p first."""
    qv, qw = q[:3], q[3]
    pv, pw = p[:3], p[3]
    return np.concatenate([qw * pv + pw * qv + np.cross(qv, pv), [qw * pw - qv @ pv]])


def quat_rotate(q: npt.NDArray[np.float64], v: npt.NDArray[np.float64]) -> npt.NDArray[np.float64]:
    qv, qw = q[:3], q[3]
    return v + 2.0 * qw * np.cross(qv, v) + 2.0 * np.cross(qv, np.cross(qv, v))


def derivative(
    params: ModelParams,
    commands: npt.NDArray[np.float64],
    state: npt.NDArray[np.float64],
) -> npt.NDArray[np.float64]:
    """Time derivative, mirroring FDroneFlightModel::Derivative (quiet settings:
    no wind, no gust, ground effect inert)."""
    velocity = state[3:6]
    attitude = state[6:10]
    angular = state[10:13]
    motors = state[13:17]

    rho_ratio = params.air_density / SEA_LEVEL_AIR_DENSITY
    clamped = np.clip(commands, 0.0, params.motor_max_rad_s)
    motor_accel = (clamped - motors) / params.motor_tau_s

    speed_sq = motors * motors
    thrust = params.thrust_coeff * rho_ratio * speed_sq
    yaw_torque = -float(np.array(SPIN_DIRECTION) @ (params.torque_coeff * rho_ratio * speed_sq))
    gyro_momentum = float(np.array(SPIN_DIRECTION) @ motors)

    lever = params.arm_length_m * np.sqrt(0.5)
    torque = np.array(
        [
            lever * (thrust[0] - thrust[1] - thrust[2] + thrust[3]),
            lever * (-thrust[0] - thrust[1] + thrust[2] + thrust[3]),
            yaw_torque,
        ]
    )
    torque -= params.rotor_inertia * gyro_momentum * np.cross(angular, np.array([0.0, 0.0, 1.0]))

    inertia = np.array([params.inertia_xx, params.inertia_yy, params.inertia_zz])
    net_torque = torque - np.cross(angular, inertia * angular)
    angular_accel = net_torque / inertia

    thrust_world = quat_rotate(attitude, np.array([0.0, 0.0, float(np.sum(thrust))]))
    drag = -(params.drag_linear * velocity + params.drag_quadratic * float(np.linalg.norm(velocity)) * velocity)
    accel = (thrust_world + drag) / params.mass_kg + np.array([0.0, 0.0, -params.gravity])

    attitude_rate = 0.5 * quat_mult(attitude, np.concatenate([angular, [0.0]]))
    return np.concatenate([velocity, accel, attitude_rate, angular_accel, motor_accel])


def main() -> int:
    csv_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(DEFAULT_CSV)
    params, samples = load_reference(csv_path)

    hover = params.hover_rad_s
    commands = hover * np.array([1.02, 0.99, 1.01, 0.98])
    initial = np.zeros(17)
    initial[0:3] = [0.0, 0.0, 50.0]
    initial[6:10] = [0.0, 0.0, np.sin(params.yaw_rad / 2.0), np.cos(params.yaw_rad / 2.0)]
    initial[13:17] = hover

    times: npt.NDArray[np.float64] = samples[:, 0]
    solution = solve_ivp(
        lambda _t, y: derivative(params, commands, y),
        (0.0, float(times[-1])),
        initial,
        method="DOP853",
        t_eval=times,
        rtol=1e-12,
        atol=1e-14,
    )
    if not solution.success:
        raise RuntimeError(f"scipy integration failed: {solution.message}")

    worst: dict[str, float] = {"pos": 0.0, "vel": 0.0, "quat": 0.0, "rate": 0.0, "motor": 0.0}
    print(f"{'t':>5} {'pos err (m)':>12} {'vel err':>12} {'quat err':>12} {'rate err':>12} {'motor err':>12}")
    for index, time_s in enumerate(times):
        cpp = samples[index]
        ref = solution.y[:, index]
        quat = ref[6:10] / np.linalg.norm(ref[6:10])
        if quat @ cpp[7:11] < 0.0:  # quaternion double cover: compare up to sign
            quat = -quat
        errors = {
            "pos": float(np.linalg.norm(cpp[1:4] - ref[0:3])),
            "vel": float(np.linalg.norm(cpp[4:7] - ref[3:6])),
            "quat": float(np.linalg.norm(cpp[7:11] - quat)),
            "rate": float(np.linalg.norm(cpp[11:14] - ref[10:13])),
            "motor": float(np.linalg.norm(cpp[14:18] - ref[13:17])),
        }
        for key, value in errors.items():
            worst[key] = max(worst[key], value)
        print(
            f"{time_s:5.1f} {errors['pos']:12.3e} {errors['vel']:12.3e}"
            f" {errors['quat']:12.3e} {errors['rate']:12.3e} {errors['motor']:12.3e}"
        )

    passed = (
        worst["pos"] < TOL_POSITION_M
        and worst["vel"] < TOL_VELOCITY_MS
        and worst["quat"] < TOL_QUATERNION
        and worst["rate"] < TOL_ANGULAR_RATE
        and worst["motor"] < TOL_MOTOR_RADS
    )
    print(f"\nworst errors: {worst}")
    print(f"cross-validation {'PASSED' if passed else 'FAILED'}")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
