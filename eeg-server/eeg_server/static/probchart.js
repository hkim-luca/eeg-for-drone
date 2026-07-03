/* Per-action probability time series: one fixed categorical color per action
 * (never reassigned), legend always present, crosshair tooltip on hover. */
"use strict";

const PROB_SERIES_VARS = ["--series-1", "--series-2", "--series-3", "--series-4", "--series-5"];

let probState = { order: [], history: [] };
let probLegendBuilt = false;

function probColor(index) {
  return getComputedStyle(document.body).getPropertyValue(PROB_SERIES_VARS[index]).trim();
}

function buildProbLegend(order) {
  const legend = document.getElementById("prob-legend");
  legend.innerHTML = "";
  order.forEach((action, index) => {
    const item = document.createElement("span");
    item.className = "legend-item";
    const chip = document.createElement("span");
    chip.className = "legend-chip";
    chip.style.background = probColor(index);
    item.append(chip, document.createTextNode(action));
    legend.append(item);
  });
  probLegendBuilt = true;
}

function drawProbChart(order, history) {
  probState = { order, history };
  if (!probLegendBuilt && order.length > 0) buildProbLegend(order);

  const canvas = document.getElementById("prob-chart");
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }

  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, width, height);

  // recessive horizontal grid at 0/25/50/75/100%
  ctx.strokeStyle = css("--grid");
  ctx.fillStyle = css("--text-muted");
  ctx.font = "10px system-ui, sans-serif";
  ctx.lineWidth = 1;
  for (let percent = 0; percent <= 100; percent += 25) {
    const y = Math.round(height - 12 - (percent / 100) * (height - 24)) - 0.5;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
    ctx.fillText(`${percent}%`, 4, y - 3);
  }

  if (history.length < 2) return;
  const capacity = 300; // config.PROB_HISTORY_LENGTH: fixed window so the chart scrolls
  const stepX = width / (capacity - 1);
  const offset = capacity - history.length;

  order.forEach((action, series) => {
    ctx.strokeStyle = probColor(series);
    ctx.lineWidth = 2;
    ctx.beginPath();
    history.forEach((entry, index) => {
      const x = (offset + index) * stepX;
      const y = height - 12 - (entry[series] / 100) * (height - 24);
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  });
}

function initProbTooltip() {
  const canvas = document.getElementById("prob-chart");
  const tooltip = document.getElementById("prob-tooltip");

  canvas.addEventListener("mousemove", (event) => {
    const { order, history } = probState;
    if (history.length < 2) { tooltip.hidden = true; return; }

    const rect = canvas.getBoundingClientRect();
    const capacity = 300;
    const offset = capacity - history.length;
    const index = Math.round((event.clientX - rect.left) / rect.width * (capacity - 1)) - offset;
    if (index < 0 || index >= history.length) { tooltip.hidden = true; return; }

    const rows = order.map((action, series) =>
      `<div class="row"><span class="legend-chip" style="background:${probColor(series)}"></span>` +
      `${action}<span class="val">${history[index][series].toFixed(1)}%</span></div>`);
    tooltip.innerHTML = rows.join("");
    tooltip.hidden = false;
    const x = Math.min(event.clientX - rect.left + 12, rect.width - tooltip.offsetWidth - 4);
    tooltip.style.left = `${Math.max(0, x)}px`;
    tooltip.style.top = "10px";
  });
  canvas.addEventListener("mouseleave", () => { tooltip.hidden = true; });
}

initProbTooltip();
