/* Polls /api/state and renders the KPI tiles, the accuracy sparkline and the
 * 32-channel waveform strips. Single-hue rendering (series-1) per the palette:
 * channels are one signal source, not distinct series. */
"use strict";

const POLL_INTERVAL_MS = 500;
const ACCURACY_HISTORY = 120; // ~1 minute of sparkline
const FULL_SCALE_UV = 40;     // waveform amplitude mapped to a strip half-height

const accuracyHistory = [];

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

function renderMetrics(state) {
  const metrics = state.metrics;
  setText("accuracy", fmt(metrics.accuracy.percent));
  setText("accuracy-window", String(metrics.accuracy.window));

  const infer = metrics.latency_ms.infer_to_control;
  setText("latency-mean", fmt(infer.mean));
  setText("latency-last", fmt(infer.last));
  setText("latency-p95", fmt(infer.p95));

  const device = metrics.latency_ms.device_to_control;
  setText("e2e-mean", fmt(device.mean));
  setText("e2e-last", fmt(device.last));
  setText("e2e-p95", fmt(device.p95));

  const reliability = metrics.reliability;
  const overall = Math.min(reliability.frame_percent, reliability.ack_percent);
  setText("reliability", fmt(overall));
  setText("frame-percent", fmt(reliability.frame_percent));
  setText("frames-lost", String(reliability.frames_lost));
  setText("ack-percent", fmt(reliability.ack_percent));

  setText("true-action", state.true_action);
  setText("inferred-action", state.inferred_action);
  setText("confidence", state.confidence.toFixed(2));
}

function drawSparkline(percent) {
  accuracyHistory.push(percent);
  if (accuracyHistory.length > ACCURACY_HISTORY) accuracyHistory.shift();

  const canvas = document.getElementById("accuracy-spark");
  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (accuracyHistory.length < 2) return;

  ctx.strokeStyle = css("--series-1");
  ctx.lineWidth = 2;
  ctx.beginPath();
  const stepX = canvas.width / (ACCURACY_HISTORY - 1);
  accuracyHistory.forEach((value, index) => {
    const x = index * stepX;
    const y = canvas.height - 2 - (value / 100) * (canvas.height - 4);
    if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function drawWaveforms(waveforms) {
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
  const stripHeight = height / channels;
  const amplitudeScale = (stripHeight * 0.45) / FULL_SCALE_UV;

  // recessive strip separators and channel labels every 4th electrode
  ctx.strokeStyle = css("--grid");
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  ctx.lineWidth = 1;
  for (let channel = 0; channel < channels; channel += 1) {
    const y = Math.round((channel + 1) * stripHeight) - 0.5;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
    if (channel % 4 === 0) {
      ctx.fillText(`ch${channel}`, 4, channel * stripHeight + 11);
    }
  }

  ctx.strokeStyle = css("--series-1");
  ctx.lineWidth = 1;
  for (let channel = 0; channel < channels; channel += 1) {
    const samples = waveforms[channel];
    if (samples.length < 2) continue;
    const centerY = (channel + 0.5) * stripHeight;
    const stepX = width / (samples.length - 1);
    ctx.beginPath();
    samples.forEach((value, index) => {
      const y = centerY - Math.max(-1, Math.min(1, value / FULL_SCALE_UV)) * stripHeight * 0.45;
      if (index === 0) ctx.moveTo(index * stepX, y); else ctx.lineTo(index * stepX, y);
    });
    ctx.stroke();
  }
}

async function poll() {
  try {
    const response = await fetch("/api/state", { cache: "no-store" });
    const state = await response.json();
    renderConnection(state.connected);
    renderMetrics(state);
    drawSparkline(state.metrics.accuracy.percent);
    drawWaveforms(state.waveforms);
  } catch (error) {
    renderConnection(false);
  } finally {
    setTimeout(poll, POLL_INTERVAL_MS);
  }
}

poll();
