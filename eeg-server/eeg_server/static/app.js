/* Polls /api/state and renders the KPI tiles and the 32-channel waveform
 * strips. Single-hue rendering (series-1) per the palette: channels are one
 * signal source, not distinct series. */
"use strict";

const POLL_INTERVAL_MS = 500;
const FULL_SCALE_UV = 40;     // waveform amplitude mapped to a strip half-height

function css(name) {
  return getComputedStyle(document.body).getPropertyValue(name).trim();
}

function setText(id, text) {
  document.getElementById(id).textContent = text;
}

function fmt(value) {
  return Number.isFinite(value) ? value.toFixed(1) : "--";
}

function renderConnection(connected) {
  const badge = document.getElementById("connection");
  badge.classList.toggle("status-up", connected);
  badge.classList.toggle("status-down", !connected);
  setText("connection-label", connected ? "DroneSim 연결됨" : "DroneSim 연결 끊김 (자동 재접속 대기)");
}

function renderPipelineSummary(state) {
  const device = state.metrics.latency_ms.device_to_control;
  setText("pipeline-total-ms", fmt(Math.max(device.last, 0)));
  setText("pipeline-mean-ms", fmt(device.mean));
  setText("pipeline-p95-ms", fmt(device.p95));
}

function renderMetrics(state) {
  const metrics = state.metrics;
  const reliability = metrics.reliability;
  const overall = Math.min(reliability.frame_percent, reliability.ack_percent);
  setText("reliability", fmt(overall));
  setText("frame-percent", fmt(reliability.frame_percent));
  setText("frames-lost", String(reliability.frames_lost));
  setText("ack-percent", fmt(reliability.ack_percent));

  setText("inferred-action", state.inferred_action);
  setText("confidence", state.confidence.toFixed(2));
}

// proto field name -> [group, label, unit, decimals]; grouped like the in-game settings UI
const PHYSICS_FIELDS = {
  mass_kg:               ["기체", "질량", "kg", 3],
  arm_length_m:          ["기체", "암 길이", "m", 3],
  inertia_xx:            ["기체", "관성 Ixx", "kg·m²", 5],
  inertia_yy:            ["기체", "관성 Iyy", "kg·m²", 5],
  inertia_zz:            ["기체", "관성 Izz", "kg·m²", 5],
  motor_count:           ["모터", "모터 수", "", 0],
  motor_time_constant_s: ["모터", "시정수", "s", 3],
  thrust_coefficient:    ["모터", "추력 계수 kT", "", 9],
  torque_coefficient:    ["모터", "토크 계수 kQ", "", 10],
  motor_max_rad_s:       ["모터", "최대 회전속도", "rad/s", 0],
  rotor_inertia:         ["모터", "로터 관성", "kg·m²", 7],
  rotor_radius_m:        ["모터", "로터 반경", "m", 3],
  gravity:               ["환경", "중력", "m/s²", 3],
  air_density:           ["환경", "공기밀도", "kg/m³", 3],
  drag_linear:           ["환경", "항력(선형)", "N/(m/s)", 3],
  drag_quadratic:        ["환경", "항력(이차)", "N/(m/s)²", 3],
  ground_effect_strength:["환경", "지면 효과", "", 2],
  wind_x:                ["환경", "바람 X", "m/s", 1],
  wind_y:                ["환경", "바람 Y", "m/s", 1],
  gust_intensity:        ["환경", "돌풍 강도", "m/s", 1],
  max_speed_ms:          ["제어", "최대 속도", "m/s", 1],
  max_tilt_deg:          ["제어", "최대 기울기", "°", 1],
  substep_hz:            ["제어", "적분 주파수", "Hz", 0],
};
const PHYSICS_GROUPS = ["기체", "모터", "환경", "제어"];

function renderPhysicsSettings(settings) {
  const grid = document.getElementById("physics-grid");
  const empty = document.getElementById("physics-empty");
  const hasData = settings && Object.keys(settings).length > 0;
  grid.hidden = !hasData;
  empty.hidden = hasData;
  setText("physics-preset", hasData ? (settings.preset_name ?? "") : "");
  if (!hasData) return;

  // proto3 omits zero-valued fields, so show 0 for known-but-absent ones
  const groups = new Map(PHYSICS_GROUPS.map((name) => [name, []]));
  for (const [field, [group, label, unit, decimals]] of Object.entries(PHYSICS_FIELDS)) {
    const value = settings[field] ?? 0;
    const text = Number(value).toFixed(decimals) + (unit ? ` ${unit}` : "");
    groups.get(group).push(`<div class="physics-item"><span>${label}</span><span class="val">${text}</span></div>`);
  }

  grid.innerHTML = PHYSICS_GROUPS
    .map((name) => `<div class="physics-group"><h3>${name}</h3>${groups.get(name).join("")}</div>`)
    .join("");
}

function kstTime(date) {
  return date.toLocaleTimeString("ko-KR", { timeZone: "Asia/Seoul", hour12: false });
}

function drawWaveforms(waveforms, channelNames) {
  const canvas = document.getElementById("waveforms");
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }

  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, width, height);

  const channels = waveforms.length;
  if (channels === 0) return;

  // reserve a left gutter for channel-name labels so the signal lines never draw over them
  const labelWidth = 32;
  const axisHeight = 18;
  const plotLeft = labelWidth;
  const plotWidth = width - labelWidth;
  const plotHeight = height - axisHeight;

  // KST time axis along the bottom: the window is the last 2 seconds
  const now = Date.now();
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  [[plotLeft, "left", -2000], [plotLeft + plotWidth / 2, "center", -1000], [plotLeft + plotWidth, "right", 0]]
    .forEach(([x, align, offsetMs]) => {
      ctx.textAlign = align;
      ctx.fillText(`${kstTime(new Date(now + offsetMs))}`, x, height - 5);
    });
  ctx.textAlign = "left";

  const stripHeight = plotHeight / channels;

  // recessive strip separators across the plot area, and one channel label per electrode in
  // the reserved left gutter, vertically centered on its own strip
  ctx.strokeStyle = css("--grid");
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  ctx.lineWidth = 1;
  for (let channel = 0; channel < channels; channel += 1) {
    const y = Math.round((channel + 1) * stripHeight) - 0.5;
    ctx.beginPath();
    ctx.moveTo(plotLeft, y);
    ctx.lineTo(width, y);
    ctx.stroke();
    ctx.fillText(channelNames[channel] ?? `ch${channel}`, 2, (channel + 0.5) * stripHeight + 3);
  }

  ctx.strokeStyle = css("--series-1");
  ctx.lineWidth = 1;
  for (let channel = 0; channel < channels; channel += 1) {
    const samples = waveforms[channel];
    if (samples.length < 2) continue;
    const centerY = (channel + 0.5) * stripHeight;
    const stepX = plotWidth / (samples.length - 1);
    ctx.beginPath();
    samples.forEach((value, index) => {
      const x = plotLeft + index * stepX;
      const y = centerY - Math.max(-1, Math.min(1, value / FULL_SCALE_UV)) * stripHeight * 0.45;
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
}

async function poll() {
  try {
    const response = await fetch("/api/state", { cache: "no-store" });
    const state = await response.json();
    renderConnection(state.connected);
    renderPipelineSummary(state);
    renderMetrics(state);
    drawProbChart(state.prob_order, state.prob_history, state.prob_times);
    drawPipelineChart(state.latency_history, state.latency_times);
    drawWaveforms(state.waveforms, state.channel_names);
    renderPhysicsSettings(state.physics_settings);
  } catch (error) {
    renderConnection(false);
  } finally {
    setTimeout(poll, POLL_INTERVAL_MS);
  }
}

poll();
