/* Response-path latency as a line chart over time (one line per segment): device->infer,
 * infer processing, infer->control, and any unaccounted overhead. Mirrors the DroneSim HUD
 * widget's pipeline chart so both surfaces agree. */
"use strict";

const PIPELINE_SERIES = [
  { key: "device_infer", label: "device→추론", cssVar: "--series-1" },
  { key: "infer", label: "추론", cssVar: "--series-3" },
  { key: "infer_control", label: "추론→제어", cssVar: "--series-2" },
  { key: "overhead", label: "overhead", cssVar: "--text-muted" },
  { key: "total", label: "전체", cssVar: "--text-primary" },
];

let pipelineLegendBuilt = false;

const PIPELINE_TOP_MARGIN = 4;
const PIPELINE_BOTTOM_MARGIN = 20; // leaves room for the KST time axis

function pipelineColor(series) {
  return getComputedStyle(document.body).getPropertyValue(series.cssVar).trim();
}

function buildPipelineLegend() {
  const legend = document.getElementById("pipeline-legend");
  legend.innerHTML = "";
  PIPELINE_SERIES.forEach((series) => {
    const item = document.createElement("span");
    item.className = "legend-item";
    const chip = document.createElement("span");
    chip.className = "legend-chip";
    chip.style.background = pipelineColor(series);
    const value = document.createElement("span");
    value.id = `pipeline-legend-value-${series.key}`;
    value.textContent = "-- ms";
    item.append(chip, document.createTextNode(`${series.label} `), value);
    legend.append(item);
  });
  pipelineLegendBuilt = true;
}

function drawPipelineChart(history, times) {
  if (!pipelineLegendBuilt) buildPipelineLegend();

  const canvas = document.getElementById("pipeline-chart");
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }

  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, width, height);
  if (history.length < 2) return;

  const plotTop = PIPELINE_TOP_MARGIN;
  const plotHeight = height - PIPELINE_TOP_MARGIN - PIPELINE_BOTTOM_MARGIN;
  const plotBottom = plotTop + plotHeight;

  // scale to the tallest single segment value in the visible window, with headroom
  const maxValue = Math.max(...history.flat(), 1) * 1.1;

  const capacity = 300; // config.PROB_HISTORY_LENGTH: fixed window so the chart scrolls
  const stepX = width / (capacity - 1);
  const offset = capacity - history.length;
  const colors = PIPELINE_SERIES.map(pipelineColor);

  PIPELINE_SERIES.forEach((series, seriesIndex) => {
    ctx.strokeStyle = colors[seriesIndex];
    ctx.lineWidth = series.lineWidth ?? 1.5;
    ctx.beginPath();
    history.forEach((entry, index) => {
      const x = (offset + index) * stepX;
      const y = plotBottom - (entry[seriesIndex] / maxValue) * plotHeight;
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();

    // current value, shown in the legend (name + value, color-matched) rather than on the
    // canvas - same "stat unit" style as the DroneSim HUD widget
    const lastEntry = history[history.length - 1];
    document.getElementById(`pipeline-legend-value-${series.key}`).textContent = `${fmt(lastEntry[seriesIndex])} ms`;
  });

  // KST time axis: up to four ticks spread over the visible samples
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  ctx.textAlign = "center";
  const tickCount = Math.min(4, times.length);
  for (let tick = 0; tick < tickCount; tick += 1) {
    const index = Math.round(tick * (times.length - 1) / Math.max(1, tickCount - 1));
    const x = Math.min(Math.max((offset + index) * stepX, 24), width - 24);
    ctx.fillText(times[index], x, height - 6);
  }
  ctx.textAlign = "left";
}
