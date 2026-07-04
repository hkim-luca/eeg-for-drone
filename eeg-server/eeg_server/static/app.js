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

function renderMetrics(state) {
  const metrics = state.metrics;
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

  setText("inferred-action", state.inferred_action);
  setText("confidence", state.confidence.toFixed(2));
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

  // KST time axis along the bottom: the window is the last 2 seconds
  const axisHeight = 18;
  const plotHeight = height - axisHeight;
  const now = Date.now();
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  [[0, "left", -2000], [width / 2, "center", -1000], [width, "right", 0]].forEach(([x, align, offsetMs]) => {
    ctx.textAlign = align;
    ctx.fillText(`${kstTime(new Date(now + offsetMs))}`, x, height - 5);
  });
  ctx.textAlign = "left";

  const stripHeight = plotHeight / channels;
  const amplitudeScale = (stripHeight * 0.45) / FULL_SCALE_UV;

  // recessive strip separators and one channel label per electrode
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
    ctx.fillText(channelNames[channel] ?? `ch${channel}`, 4, channel * stripHeight + 11);
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
    drawProbChart(state.prob_order, state.prob_history, state.prob_times);
    drawWaveforms(state.waveforms, state.channel_names);
  } catch (error) {
    renderConnection(false);
  } finally {
    setTimeout(poll, POLL_INTERVAL_MS);
  }
}

poll();
