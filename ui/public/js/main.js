import * as Juce from "./juce/index.js";

// ---------------------------------------------------------------------------
// Parameter states. Names match the IDs declared in PluginProcessor.cpp.
// ---------------------------------------------------------------------------
const params = {
  time:     Juce.getSliderState("time"),
  feedback: Juce.getSliderState("feedback"),
  mix:      Juce.getSliderState("mix"),
  tone:     Juce.getSliderState("tone"),
};
const syncState     = Juce.getToggleState("sync");
const divisionState = Juce.getComboBoxState("division");

const DIVISION_BEATS = [0.125, 1 / 6, 0.25, 0.375, 1 / 3, 0.5, 0.75, 2 / 3, 1.0, 1.5, 2.0];

// ---------------------------------------------------------------------------
// Knobs
// ---------------------------------------------------------------------------
const ARC_LENGTH = 179; // 270 degrees of the r=38 circle used in the markup

function formatValue(state) {
  const v = state.getScaledValue();
  const unit = state.properties.label || "";
  if (unit === "Hz") return v >= 1000 ? (v / 1000).toFixed(2) + " kHz" : Math.round(v) + " Hz";
  if (unit === "ms") return v >= 1000 ? (v / 1000).toFixed(2) + " s" : Math.round(v) + " ms";
  if (unit === "%")  return Math.round(v) + " %";
  return v.toFixed(2);
}

function wireKnob(el) {
  const id = el.dataset.param;
  const state = params[id];
  const valueArc = el.querySelector(".value");
  const pointer = el.querySelector(".pointer");
  const readout = document.querySelector(`[data-readout="${id}"]`);

  function render() {
    const n = state.getNormalisedValue();
    valueArc.style.strokeDasharray = `${n * ARC_LENGTH} 240`;
    pointer.style.transform = `rotate(${135 + n * 270}deg)`;
    if (readout) readout.textContent = formatValue(state);
  }

  // Vertical drag. 180px of travel covers the full range; holding shift is finer.
  let dragging = false;
  let lastY = 0;

  el.addEventListener("pointerdown", (e) => {
    dragging = true;
    lastY = e.clientY;
    el.classList.add("active");
    el.setPointerCapture(e.pointerId);
  });

  el.addEventListener("pointermove", (e) => {
    if (!dragging) return;
    const speed = e.shiftKey ? 600 : 180;
    const delta = (lastY - e.clientY) / speed;
    lastY = e.clientY;
    state.setNormalisedValue(Math.min(1, Math.max(0, state.getNormalisedValue() + delta)));
    render();
  });

  function endDrag(e) {
    if (!dragging) return;
    dragging = false;
    el.classList.remove("active");
    el.releasePointerCapture(e.pointerId);
  }
  el.addEventListener("pointerup", endDrag);
  el.addEventListener("pointercancel", endDrag);

  state.valueChangedEvent.addListener(render);
  state.propertiesChangedEvent.addListener(render);
  render();
}

document.querySelectorAll(".knob").forEach(wireKnob);

// ---------------------------------------------------------------------------
// Sync toggle and Division stepper
// ---------------------------------------------------------------------------
const syncButton    = document.getElementById("sync");
const timeGroup     = document.getElementById("timeGroup");
const divisionGroup = document.getElementById("divisionGroup");
const divisionValue = document.getElementById("divisionValue");
const divisionMs    = document.getElementById("divisionMs");

function divisionNames() {
  return divisionState.properties.choices || [];
}

function renderSync() {
  const on = syncState.getValue();
  syncButton.setAttribute("aria-checked", on ? "true" : "false");
  // Only one of Time / Division is meaningful at a time, so only show that one.
  timeGroup.classList.toggle("hidden", on);
  divisionGroup.classList.toggle("hidden", !on);
  document.getElementById("syncMode").textContent = on ? "Tempo" : "Free";
  renderDivision();
}

function renderDivision() {
  const names = divisionNames();
  const index = divisionState.getChoiceIndex();
  divisionValue.textContent = names[index] ?? "--";

  // The UI has no access to host tempo, so show the division's length at the
  // 120 BPM the processor falls back to, labelled so it isn't mistaken for truth.
  const beats = DIVISION_BEATS[index];
  if (beats !== undefined) {
    divisionMs.textContent = Math.round((beats * 60000) / 120) + " ms @120";
  }
}

syncButton.addEventListener("click", () => {
  syncState.setValue(!syncState.getValue());
  renderSync();
});

document.querySelectorAll(".step").forEach((button) => {
  button.addEventListener("click", () => {
    const names = divisionNames();
    const next = divisionState.getChoiceIndex() + Number(button.dataset.step);
    if (next >= 0 && next < names.length) {
      divisionState.setChoiceIndex(next);
      renderDivision();
    }
  });
});

syncState.valueChangedEvent.addListener(renderSync);
divisionState.valueChangedEvent.addListener(renderDivision);
divisionState.propertiesChangedEvent.addListener(renderDivision);
renderSync();

// ---------------------------------------------------------------------------
// The orbit field
//
// Each repeat is a body orbiting the dry signal at the centre. Orbit radius
// tracks delay time, the number of visible bodies tracks feedback, their colour
// tracks Tone, and their size tracks Mix. Nothing here touches audio: it is
// driven entirely by parameter values.
// ---------------------------------------------------------------------------
const canvas = document.getElementById("orbit");
const ctx = canvas.getContext("2d");

function resizeCanvas() {
  const dpr = window.devicePixelRatio || 1;
  canvas.width = 640 * dpr;
  canvas.height = 420 * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
resizeCanvas();
window.addEventListener("resize", resizeCanvas);

const CENTRE_X = 320;
const CENTRE_Y = 150;
const ORBIT_TILT = 0.55; // how far the orbit is tipped away from us

function currentDelaySeconds() {
  if (syncState.getValue()) {
    const beats = DIVISION_BEATS[divisionState.getChoiceIndex()] ?? 0.5;
    return (beats * 60) / 120;
  }
  return params.time.getScaledValue() / 1000;
}

// Colour temperature, the way real colour temperature works: ember through
// white to ice. Interpolating hue directly would run through green, which is
// in neither the palette nor the metaphor.
const TEMPERATURE = [
  [0.0, [255, 138, 66]],   // ember
  [0.55, [242, 240, 246]], // white hot
  [1.0, [111, 211, 255]],  // ice
];

function bodyColour(toneNorm, alpha) {
  let [x0, c0] = TEMPERATURE[0];
  let [x1, c1] = TEMPERATURE[TEMPERATURE.length - 1];

  for (let i = 0; i < TEMPERATURE.length - 1; i++) {
    if (toneNorm >= TEMPERATURE[i][0] && toneNorm <= TEMPERATURE[i + 1][0]) {
      [x0, c0] = TEMPERATURE[i];
      [x1, c1] = TEMPERATURE[i + 1];
      break;
    }
  }

  const t = x1 === x0 ? 0 : (toneNorm - x0) / (x1 - x0);
  const ch = (i) => Math.round(c0[i] + (c1[i] - c0[i]) * t);
  return `rgba(${ch(0)}, ${ch(1)}, ${ch(2)}, ${alpha})`;
}

let angle = 0;
let lastFrame = performance.now();

function draw(now) {
  const dt = Math.min(0.05, (now - lastFrame) / 1000);
  lastFrame = now;

  const delaySeconds = currentDelaySeconds();
  const feedback = params.feedback.getNormalisedValue();
  const mix = params.mix.getNormalisedValue();
  const toneNorm = params.tone.getNormalisedValue();

  // A shorter delay means a faster orbit, so the picture reads as "rate".
  angle += (dt / Math.max(0.05, delaySeconds)) * Math.PI * 2 * 0.25;

  ctx.clearRect(0, 0, 640, 420);

  // Radius maps delay time (1ms..2s of useful range) onto the panel.
  const radius = 62 + Math.min(1, Math.sqrt(delaySeconds / 2)) * 128;

  // Orbit path - an ellipse, matching the tilt the bodies actually travel on.
  ctx.beginPath();
  ctx.ellipse(CENTRE_X, CENTRE_Y, radius, radius * ORBIT_TILT, 0, 0, Math.PI * 2);
  ctx.strokeStyle = "rgba(111, 211, 255, 0.13)";
  ctx.lineWidth = 1;
  ctx.stroke();

  // How many repeats stay audible before decaying below hearing.
  const gain = feedback * 0.95;
  const visible = gain < 0.02 ? 1 : Math.min(14, Math.ceil(Math.log(0.02) / Math.log(gain)));

  // Keep the whole tail inside one loop of the orbit: a repeat lapping the
  // orbit and overlapping a newer one would misrepresent what the delay does.
  const spacing = Math.min(0.55, (Math.PI * 2 * 0.86) / Math.max(1, visible));

  for (let i = visible - 1; i >= 0; i--) {
    const decay = Math.pow(gain, i);
    // Floor the fade so a long tail still reads as a trail of bodies rather
    // than vanishing after the second repeat.
    const alpha = Math.max(0.16, decay) * (0.4 + mix * 0.6);
    const bodyAngle = angle - i * spacing;
    const x = CENTRE_X + Math.cos(bodyAngle) * radius;
    const y = CENTRE_Y + Math.sin(bodyAngle) * radius * ORBIT_TILT;
    const size = (3.5 + mix * 5) * (0.55 + decay * 0.45);

    const glow = ctx.createRadialGradient(x, y, 0, x, y, size * 4);
    glow.addColorStop(0, bodyColour(toneNorm, alpha));
    glow.addColorStop(1, bodyColour(toneNorm, 0));
    ctx.fillStyle = glow;
    ctx.beginPath();
    ctx.arc(x, y, size * 4, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = bodyColour(toneNorm, Math.min(1, alpha + 0.25));
    ctx.beginPath();
    ctx.arc(x, y, size, 0, Math.PI * 2);
    ctx.fill();
  }

  // The dry signal: a point source at the centre, dimming as Mix goes wet.
  const coreAlpha = 0.3 + (1 - mix) * 0.7;
  const core = ctx.createRadialGradient(CENTRE_X, CENTRE_Y, 0, CENTRE_X, CENTRE_Y, 26);
  core.addColorStop(0, `rgba(255, 255, 255, ${coreAlpha})`);
  core.addColorStop(0.25, `rgba(226, 234, 252, ${coreAlpha * 0.5})`);
  core.addColorStop(1, "rgba(198, 214, 250, 0)");
  ctx.fillStyle = core;
  ctx.beginPath();
  ctx.arc(CENTRE_X, CENTRE_Y, 26, 0, Math.PI * 2);
  ctx.fill();

  requestAnimationFrame(draw);
}

requestAnimationFrame(draw);
