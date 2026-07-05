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
  } catch (error) {
    renderConnection(false);
  } finally {
    setTimeout(poll, POLL_INTERVAL_MS);
  }
}

poll();
