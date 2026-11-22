"use strict";
const $ = (id) => document.getElementById(id);
const canvas = $("canvas"),
  sheet = $("sheet"),
  content = $("content");
var Module = {
  canvas,
  printErr: (text) => console.error(text),
  onAbort: (text) => window.mountainError(String(text)),
};
const state = {
  ready: false,
  camera: false,
  keys: new Set(),
  stick: [0, 0],
  photos: [],
  db: null,
  toastTimer: 0,
};
const canLockMouse = typeof canvas.requestPointerLock === "function";
function mouseLocked() {
  return document.pointerLockElement === canvas;
}
function releaseMouse() {
  if (!mouseLocked()) return false;
  document.exitPointerLock();
  return true;
}
function canLook() {
  return (
    state.ready &&
    !sheet.open &&
    !document.hidden &&
    !window.mountainXR?.session
  );
}
const val = (id) => Module._mountain_value(id);
const nameOf = (id) =>
  Module.ccall("mountain_label", "string", ["number"], [id]);
const icon = (name) => '<i aria-hidden="true" class="icon i-' + name + '"></i>';
const phenomenon = (index) =>
  Module.ccall("mountain_option", "string", ["number", "number"], [2, index]);
const near = [
  "",
  "Open door",
  "Step inside",
  "Step outside",
  "Next room",
  "Previous room",
  "Read field sign",
  "Discover treasure",
  "Open room door",
  "Open window",
  "Close window",
  "Open balcony door",
  "Cross balcony threshold",
  "Gather",
  "Open kitchen",
  "Sit down",
  "Stand up",
  "Tend winter garden",
];
function notice(message) {
  clearTimeout(state.toastTimer);
  $("toast").textContent = message;
  $("toast").classList.add("shown");
  state.toastTimer = setTimeout(
    () => $("toast").classList.remove("shown"),
    Math.max(3100, Math.min(8500, message.length * 55)),
  );
}
function timeText(hour) {
  const m = Math.floor(hour * 60) % 1440;
  return (
    String(Math.floor(m / 60)).padStart(2, "0") +
    ":" +
    String(m % 60).padStart(2, "0")
  );
}
function resize() {
  if (!state.ready) return;
  const area = $("viewport").getBoundingClientRect();
  const scale = Math.min(1, 700 / area.width, 430 / area.height);
  const w = Math.max(2, Math.round(area.width * scale)),
    h = Math.max(2, Math.round(area.height * scale));
  if (canvas.width !== w || canvas.height !== h) Module._mountain_resize(w, h);
}
const points = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
function compass(deg) {
  const bearing = $("bearing"),
    step = 5,
    first = Math.floor(deg / step) - 9;
  if (!bearing.children.length) {
    for (let i = 0; i < 20; i++)
      bearing.appendChild(document.createElement("span"));
  }
  Array.from(bearing.children).forEach((el, i) => {
    const mark = (first + i) * step,
      normalized = ((mark % 360) + 360) % 360;
    el.textContent =
      normalized % 45 === 0
        ? points[normalized / 45]
        : normalized % 15 === 0
          ? normalized
          : "";
    el.className = el.textContent ? "" : "minor";
    el.style.left = "calc(50% + " + (mark - deg) * 3.5 + "px)";
  });
  $("compass").setAttribute(
    "aria-label",
    "Heading " + Math.round((deg + 360) % 360) + " degrees",
  );
}
function refresh() {
  if (!state.ready) return;
  const inside = val(11),
    h = val(4),
    weather = val(6),
    use = near[val(13)];
  $("location").textContent = inside ? nameOf(3) : nameOf(0);
  $("biome").textContent = inside
    ? nameOf(12) + " · Room " + (val(12) + 1)
    : val(36) > 1
      ? nameOf(9)
      : val(36) === 1
        ? nameOf(5) + " · Alpine trail"
        : "Alpine trail";
  const hazard = val(33);
  $("footing").hidden = !hazard || state.camera || Boolean(inside);
  const warning = [
    "",
    "Exposed edge · watch your footing",
    "Slipping",
    "Back on safe footing",
  ][hazard];
  if ($("footing").textContent !== warning) $("footing").textContent = warning;
  $("height").textContent = Math.round(val(8)).toLocaleString() + " m";
  $("season").textContent = nameOf(1);
  $("clock").textContent = timeText(h);
  $("skies").textContent = inside && val(39) ? phenomenon(val(39)) : nameOf(2);
  $("sky-icon").className =
    "icon i-" +
    (val(36) === 7
      ? "moon"
      : weather
        ? ["sun", "cloud", "rain", "mist"][weather]
        : h < 6 || h >= 19
          ? "moon"
          : "sun");
  $("use").hidden = !use || state.camera;
  $("sip").hidden = state.camera || !val(49) || !val(47);
  $("bite").hidden = state.camera || !val(49) || !val(48);
  $("sit").hidden = state.camera || !val(50) || [15, 16].includes(val(13));
  $("door").hidden = state.camera || ![2, 3, 4, 5, 12].includes(val(13));
  $("blind").hidden = state.camera || ![9, 10].includes(val(13));
  $("blind").querySelector("span").textContent = val(42)
    ? "Raise blind"
    : "Lower blind";
  $("use").querySelector("span").textContent = [6, 7].includes(val(13))
    ? use + " · " + nameOf(10)
    : val(13) === 13
      ? "Gather " + nameOf(18)
      : use;
  $("cabin").querySelector("span").textContent = inside
    ? "Hearth room"
    : val(9)
      ? "Dismiss " + nameOf(12).toLowerCase()
      : "Summon " + nameOf(12).toLowerCase();
  $("cabin").querySelector("kbd").textContent = inside ? "R" : "H";
  const on = Boolean(val(14));
  $("sound").innerHTML = icon(on ? "sound" : "mute");
  $("sound").setAttribute(
    "aria-label",
    on ? "Mute ambient sound" : "Enable ambient sound",
  );
  $("sound").setAttribute("aria-pressed", String(on));
  compass((val(2) * 180) / Math.PI);
}
function walk() {
  if (!state.ready) return;
  const k = state.keys;
  Module._mountain_input(
    Number(k.has("w") || k.has("arrowup")) -
      Number(k.has("s") || k.has("arrowdown")) +
      state.stick[0],
    Number(k.has("d")) - Number(k.has("a")) + state.stick[1],
  );
}
function stop() {
  state.keys.clear();
  state.stick = [0, 0];
  $("pad").firstElementChild.style.transform = "";
  walk();
}
function suspendInput() {
  releaseMouse();
  stop();
}
function act(action) {
  if (!state.ready) return;
  const reading = action === 2 && [6, 7].includes(val(13));
  const result = Module._mountain_action(action);
  if (reading) notice(nameOf(8));
  if (action === 1 && result)
    notice(val(9) ? nameOf(12) + " summoned." : nameOf(12) + " dismissed.");
  if (action === 11) notice(nameOf(12) + " · " + phenomenon(val(39)));
  if (action === 12 || action === 13) {
    const enabled = val(action === 12 ? 40 : 41);
    notice(
      "Watch " +
        nameOf(action + 1) +
        " · " +
        (enabled ? nameOf(action + 3) || "on" : "off"),
    );
  }
  if (action === 2 && val(51)) {
    openCookbook();
    return;
  }
  if (action === 2 && nameOf(17)) notice(nameOf(17));
  if (val(57))
    notice("Pantry save unavailable. Your basket is kept for this visit.");
  if (val(43))
    notice("Cabin save unavailable. Your rooms are still here for this visit.");
  refresh();
}
function cabin() {
  act(val(11) ? 6 : 1);
}
function mode(on) {
  if (!state.ready) return;
  state.camera = on;
  document.body.classList.toggle("camera-on", on);
  $("viewfinder").hidden = !on;
  $("kit").hidden = on;
  $("camera-kit").hidden = !on;
  refresh();
}
function openSheet(title, category, wide = false) {
  if (!state.ready) return false;
  suspendInput();
  Module._mountain_pause(1);
  $("sheet-title").textContent = title;
  $("sheet-category").textContent = category;
  content.replaceChildren();
  sheet.classList.toggle("journal-panel", wide);
  if (!sheet.open) sheet.showModal();
  return true;
}
function closeSheet() {
  if (sheet.open) sheet.close();
}
sheet.addEventListener("close", () => {
  $("explore").classList.add("selected");
  $("journal").classList.remove("selected");
  if (state.ready) {
    if (val(51)) Module._mountain_action(14);
    Module._mountain_pause(0);
  }
  stop();
});
sheet.addEventListener("click", (e) => {
  const r = sheet.getBoundingClientRect();
  if (
    e.target === sheet &&
    (e.clientX < r.left ||
      e.clientX > r.right ||
      e.clientY < r.top ||
      e.clientY > r.bottom)
  )
    closeSheet();
});
function el(tag, text, cls) {
  const e = document.createElement(tag);
  if (text !== null) e.textContent = text;
  if (cls) e.className = cls;
  return e;
}
function button(text, cls, handler, glyph) {
  const b = el("button", null, cls);
  if (glyph) b.innerHTML = icon(glyph);
  b.append(document.createTextNode(text));
  b.onclick = handler;
  return b;
}
function choose(parent, title, items, current, chooseItem) {
  const wrap = el("div", null, "field");
  wrap.append(el("span", title, "field-title"));
  const group = el("div", null);
  group.setAttribute("role", "group");
  group.setAttribute("aria-label", title);
  items.forEach((item, i) => {
    const b = button(item, "choice", () => {
      chooseItem(i);
      for (const other of group.children)
        other.setAttribute("aria-pressed", String(other === b));
      refresh();
    });
    b.setAttribute("aria-pressed", String(i === current));
    group.append(b);
  });
  wrap.append(group);
  parent.append(wrap);
}
function conditions() {
  if (!openSheet("Trail conditions", "Environment")) return;
  const options = (kind, n) =>
    Array.from({ length: n }, (_, i) =>
      Module.ccall(
        "mountain_option",
        "string",
        ["number", "number"],
        [kind, i],
      ),
    );
  const modeControl = (label, key, action) => {
    const row = el("label", null, "climate-mode");
    const input = el("input", null);
    input.type = "checkbox";
    input.checked = Boolean(val(key));
    input.onchange = () => {
      act(action);
      input.checked = Boolean(val(key));
    };
    row.append(input, document.createTextNode(label));
    content.append(row);
    return input;
  };
  choose(content, "Season", options(0, 4), val(5), (i) => {
    Module._mountain_select(0, i);
    cycle.checked = false;
    if (weatherGroup) weatherGroup.children[2].textContent = options(1, 4)[2];
  });
  const cycle = modeControl("Cycle through seasons", 18, 7);
  if (val(36) !== 7)
    choose(content, "Weather", options(1, 4), val(6), (i) => {
      Module._mountain_select(1, i);
      changing.checked = false;
    });
  const weatherGroup = content.querySelector(
    '[role="group"][aria-label="Weather"]',
  );
  const changing =
    val(36) !== 7 ? modeControl("Let the weather change", 19, 8) : null;
  if (!changing)
    content.append(el("p", "Mare Imbrium has an airless sky.", "note"));
  const field = el("div", null, "field"),
    top = el("div", null, "spread"),
    l = el("label", "Time of day");
  l.htmlFor = "time-slider";
  const display = el("time", timeText(val(4)));
  top.append(l, display);
  field.append(top);
  const input = el("input", null);
  input.type = "range";
  input.id = "time-slider";
  input.min = 0;
  input.max = 1439;
  input.step = 1;
  input.value = Math.round(val(4) * 60);
  input.oninput = () => {
    Module._mountain_conditions(val(5), val(6), Number(input.value) / 60);
    display.textContent = timeText(val(4));
    refresh();
  };
  field.append(input);
  content.append(field);
  choose(
    content,
    "Light",
    ["Dawn", "Midday", "Golden hour", "Midnight"],
    -1,
    (i) => {
      const h = [6.4, 12, 17.45, 0][i];
      Module._mountain_conditions(val(5), val(6), h);
      input.value = Math.round(h * 60);
      display.textContent = timeText(h);
    },
  );
  choose(
    content,
    nameOf(12) + " · Local phenomena",
    options(2, 4),
    val(39),
    (i) => {
      while (val(39) !== i) act(11);
    },
  );
  for (let slot = 0; slot < 2; slot++) {
    const check = modeControl(
      "Watch " + nameOf(13 + slot),
      40 + slot,
      12 + slot,
    );
    const direction = nameOf(15 + slot);
    if (direction) {
      const hint = el("p", direction, "note");
      hint.hidden = !check.checked;
      check.addEventListener("change", () => {
        hint.hidden = !check.checked;
      });
      content.append(hint);
    }
  }
  content.append(
    el(
      "p",
      "Watch these from your shelter or balcony. Marine sightings follow open water.",
      "note",
    ),
  );
}
function topography(map, showSummit = false) {
  const ctx = map.getContext("2d"),
    w = map.width,
    h = map.height,
    nx = 82,
    ny = 45;
  const trail = Array.from({ length: val(20) }, (_, i) => [
    Module._mountain_path(i, 0),
    Module._mountain_path(i, 1),
  ]);
  const hiker = [val(0), val(1)];
  const station = Math.floor(val(26)),
    nearOpening = val(27) < 285;
  const first = Math.max(0, station - 13),
    last = nearOpening
      ? Math.max(5, station + 5)
      : Math.max(first, station + 13);
  const onward = Array.from({ length: last - first + 1 }, (_, j) => [
    Module._mountain_extension(first + j, 0),
    Module._mountain_extension(first + j, 1),
  ]);
  const ascent = Array.from({ length: val(31) }, (_, i) => [
    Module._mountain_ascent(i, 0),
    Module._mountain_ascent(i, 1),
  ]);
  const coordinates = showSummit
      ? [...ascent, ...trail]
      : nearOpening
        ? [...trail, ...onward, hiker]
        : [hiker],
    xs = coordinates.map((p) => p[0]),
    zs = coordinates.map((p) => p[1]);
  const minX = Math.min(...xs),
    maxX = Math.max(...xs),
    minZ = Math.min(...zs),
    maxZ = Math.max(...zs);
  const step = Math.max(
    nearOpening ? 2.75 : 5,
    (maxX - minX + 56) / nx,
    (maxZ - minZ + 52) / ny,
  );
  const ox = (minX + maxX) / 2,
    oz = (minZ + maxZ) / 2,
    alt = new Float32Array((nx + 1) * (ny + 1));
  for (let j = 0; j <= ny; j++)
    for (let i = 0; i <= nx; i++)
      alt[j * (nx + 1) + i] = Module._mountain_height(
        ox + (i - nx / 2) * step,
        oz + (j - ny / 2) * step,
      );
  const sx = w / nx,
    sy = h / ny;
  for (let j = 0; j < ny; j++)
    for (let i = 0; i < nx; i++) {
      const k = j * (nx + 1) + i,
        levels = [alt[k], alt[k + 1], alt[k + nx + 2], alt[k + nx + 1]],
        v = levels.reduce((a, b) => a + b, 0) / 4;
      ctx.fillStyle = Module._mountain_wet(
        ox + (i + 0.5 - nx / 2) * step,
        oz + (j + 0.5 - ny / 2) * step,
      )
        ? "#9bb8ad"
        : v > 95
          ? "#c6bf9c"
          : v > 40
            ? "#d4d3aa"
            : "#e0ddbb";
      ctx.fillRect(i * sx, j * sy, sx + 1, sy + 1);
      for (
        let level = Math.ceil(Math.min(...levels) / 8) * 8;
        level <= Math.max(...levels);
        level += 8
      ) {
        const pts = [],
          corners = [
            [0, 0],
            [1, 0],
            [1, 1],
            [0, 1],
          ];
        for (let e = 0; e < 4; e++) {
          const f = (e + 1) % 4;
          if (levels[e] < level === levels[f] < level) continue;
          const t = (level - levels[e]) / (levels[f] - levels[e]);
          pts.push([
            (i + corners[e][0] + (corners[f][0] - corners[e][0]) * t) * sx,
            (j + corners[e][1] + (corners[f][1] - corners[e][1]) * t) * sy,
          ]);
        }
        ctx.strokeStyle = level % 40 === 0 ? "#7f886a" : "#a5aa85";
        ctx.lineWidth = level % 40 === 0 ? 1.6 : 0.85;
        for (let p = 0; p + 1 < pts.length; p += 2) {
          ctx.beginPath();
          ctx.moveTo(...pts[p]);
          ctx.lineTo(...pts[p + 1]);
          ctx.stroke();
        }
      }
    }
  const project = ([x, z]) => [
    ((x - ox) / step) * sx + w / 2,
    ((z - oz) / step) * sy + h / 2,
  ];
  ctx.lineJoin = ctx.lineCap = "round";
  const drawPath = (line, colour = "#a14e33") => {
    ctx.beginPath();
    line
      .map(project)
      .forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.lineWidth = 5.4;
    ctx.strokeStyle = "#fff4d8";
    ctx.stroke();
    ctx.lineWidth = 2.7;
    ctx.strokeStyle = colour;
    ctx.stroke();
  };
  if (val(27) < 600) drawPath(trail);
  if (onward.length > 1) drawPath(onward);
  if (ascent.length) {
    drawPath(ascent, "#5d677f");
    for (const [point, title] of [
      [ascent[val(32)], "High Pass"],
      [ascent.at(-1), "Snow Summit"],
    ]) {
      const [x, y] = project(point);
      if (x < 7 || y < 10 || x > w - 8 || y > h - 8) continue;
      ctx.fillStyle = "#444e68";
      ctx.beginPath();
      ctx.moveTo(x, y - 6);
      ctx.lineTo(x + 6, y + 4);
      ctx.lineTo(x - 6, y + 4);
      ctx.closePath();
      ctx.fill();
      if (showSummit) {
        ctx.font = "bold 11px sans-serif";
        ctx.textAlign = x > w - 95 ? "right" : "left";
        ctx.fillText(title, x + (x > w - 95 ? -9 : 9), y + 3);
        ctx.textAlign = "left";
      }
    }
  }
  if (!nearOpening && onward.length) {
    const nearest = [...trail, ...onward, ...ascent].reduce((a, b) =>
      Math.hypot(b[0] - hiker[0], b[1] - hiker[1]) <
      Math.hypot(a[0] - hiker[0], a[1] - hiker[1])
        ? b
        : a,
    );
    const [x, y] = project(nearest);
    if (x < 4 || x > w - 4 || y < 4 || y > h - 4) {
      const dx = x - w / 2,
        dy = y - h / 2,
        scale = Math.min(
          (w / 2 - 23) / Math.abs(dx),
          (h / 2 - 20) / Math.abs(dy),
        );
      ctx.save();
      ctx.translate(w / 2 + dx * scale, h / 2 + dy * scale);
      ctx.rotate(Math.atan2(dy, dx));
      ctx.fillStyle = "#994d34";
      ctx.beginPath();
      ctx.moveTo(9, 0);
      ctx.lineTo(-6, -5);
      ctx.lineTo(-6, 5);
      ctx.fill();
      ctx.restore();
    }
  }
  for (const [index, label] of [
    [val(21), "1"],
    [trail.length - 1, "2"],
  ]) {
    const [x, y] = project(trail[index]);
    if (x < 0 || x > w || y < 0 || y > h) continue;
    ctx.beginPath();
    ctx.arc(x, y, 8, 0, 2 * Math.PI);
    ctx.fillStyle = "#83422d";
    ctx.fill();
    ctx.lineWidth = 1.5;
    ctx.strokeStyle = "#fff8df";
    ctx.stroke();
    ctx.textAlign = "center";
    ctx.font = "bold 11px sans-serif";
    ctx.fillStyle = "#fff8df";
    ctx.fillText(label, x, y + 4);
  }
  ctx.textAlign = "left";
  ctx.save();
  ctx.translate(...project(hiker));
  ctx.rotate(val(2));
  ctx.fillStyle = "#2d5344";
  ctx.strokeStyle = "#fff6df";
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(0, -11);
  ctx.lineTo(7, 8);
  ctx.lineTo(0, 5);
  ctx.lineTo(-7, 8);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  ctx.restore();
  ctx.fillStyle = "#465840";
  ctx.font = "bold 13px sans-serif";
  ctx.fillText("N ↑", 12, 19);
}
function trailMap() {
  if (!openSheet(nameOf(0), "Trail map")) return;
  const map = el("canvas", null, "map-frame");
  map.width = 490;
  map.height = 260;
  map.setAttribute(
    "aria-label",
    "Contour map of your position, nearby viewpoints and the continuing trail",
  );
  const navigation = el("div", null, "atlas-nav");
  if (val(29))
    navigation.append(button("Choose a destination", "action", atlas, "map"));
  if (val(31)) {
    let overview = false;
    const toggle = button(
      "Summit route",
      "action",
      () => {
        overview = !overview;
        topography(map, overview);
        toggle.textContent = overview ? "Local map" : "Summit route";
      },
      "compass",
    );
    navigation.append(toggle);
  }
  content.append(navigation, map);
  topography(map);
  content.append(
    el(
      "p",
      val(37)
        ? "Summit route · Blue and white blazes to High Pass and Snow Summit"
        : val(27) < 285
          ? "① " +
            nameOf(6) +
            "　 ② " +
            nameOf(7) +
            (val(31)
              ? " · Summit path branches at the second cairn"
              : " · Trail continues beyond the cairn")
          : "Endless trail · Follow the blazes between cairns",
      "route-legend",
    ),
  );
  const details = el("div", null, "map-details");
  details.append(
    el("span", Math.round(val(15) * val(24)).toLocaleString() + " m walked"),
    el("span", "Contours every " + 8 * val(24) + " m"),
  );
  content.append(details);
  const form = el("form", null, "field");
  const label = el("label", "Trail seed");
  label.htmlFor = "seed";
  const row = el("div", null, "seed-form");
  const input = el("input", null);
  input.id = "seed";
  input.type = "number";
  input.min = 0;
  input.max = 4294967295;
  input.required = true;
  input.step = 1;
  input.value = val(7);
  const load = button("Hike", "action", null, "arrow");
  load.type = "submit";
  row.append(input, load);
  form.append(label, row);
  content.append(form);
  const start = (seed, variant = false) => {
    if (variant) Module._mountain_variant(seed);
    else Module._mountain_seed(seed);
    mode(false);
    closeSheet();
    refresh();
    notice("Arrived at " + nameOf(0) + ".");
  };
  form.onsubmit = (e) => {
    e.preventDefault();
    if (form.reportValidity()) start(Number(input.value), true);
  };
  const destinations = el("div", null, "map-destinations");
  if (val(25))
    destinations.append(
      button(
        nameOf(4) + " · " + nameOf(5),
        "action",
        () => start(val(23)),
        "map",
      ),
    );
  content.append(destinations);
  destinations.append(
    button(
      val(36) <= 1 ? "Generate a new mountain" : "Generate a new trail here",
      "action primary",
      () => start(crypto.getRandomValues(new Uint32Array(1))[0], true),
      "shuffle",
    ),
  );
}
const entry = (id, field) =>
  Module.ccall("mountain_entry", "string", ["number", "number"], [id, field]);
function visit(id) {
  if (!Module._mountain_visit(id)) return;
  mode(false);
  closeSheet();
  refresh();
  notice("Arrived at " + nameOf(0) + ".");
}
function atlas() {
  if (!openSheet("Destinations", "Trail atlas", true)) return;
  for (const title of ["Earth", "Beyond Earth"]) {
    const section = el("section", null, "atlas-section");
    section.append(el("h3", title));
    const grid = el("div", null, "atlas-grid");
    for (let i = 0; i < val(29); i++) {
      if (entry(i, 3) !== title) continue;
      const name = entry(i, 0),
        region = entry(i, 1),
        card = button("", "atlas-card", () => visit(i));
      card.setAttribute("aria-label", name + " · " + region);
      if (i === val(28)) card.setAttribute("aria-current", "location");
      const photo = el("img", null);
      photo.src = "ui/atlas/" + i + ".webp";
      photo.alt = "";
      photo.width = 300;
      photo.height = 150;
      const words = el("span", null);
      words.append(el("strong", name), el("small", region));
      card.append(photo, words);
      grid.append(card);
    }
    section.append(grid);
    content.append(section);
  }
}
function guide() {
  if (!openSheet("Field guide", "Trail controls")) return;
  if (val(29)) {
    const notes = el("section", null, "expedition-note");
    notes.append(
      el("h3", val(30) === 3 ? "Discovery recorded" : "Field discovery"),
      el("p", nameOf(8)),
    );
    if (val(30) === 1) notes.append(el("p", nameOf(11), "note"));
    const found = [];
    for (let i = 0; i < val(29); i++)
      if (((val(34) >>> (i * 2)) & 3) === 3) found.push(entry(i, 2));
    if (found.length)
      notes.append(
        el("span", "Treasures found · " + found.join(" · "), "note"),
      );
    content.append(notes);
  }
  const touch = matchMedia("(pointer:coarse)").matches;
  const entries = touch
    ? [
        ["Field cookbook and balcony dining", "Tap Cookbook in the trail kit"],
        ["Walk", "Move the thumb pad"],
        ["Season and weather", "Tap the sky control"],
        ["Look around", "Drag the landscape"],
        ["Call your shelter", "Tap Summon in the trail kit"],
        [
          "Use a door or read a field sign",
          "Tap the nearby interaction prompt",
        ],
        ["Window panes and blinds", "Tap their controls beside a window"],
        ["Return to the entrance", "Tap Hearth room in the cabin"],
        ["Photograph a view", "Camera → Take photo"],
      ]
    : [
        ["Field cookbook", "Q · browse anywhere, cook in the Hearth Room"],
        ["Balcony table", "Z sit or stand · 1 sip · 2 eat"],
        ["Walk", "W A S D · ↑ ↓"],
        [
          "Look around",
          canLockMouse
            ? "Click the view to capture the mouse · Esc to release · Drag or ← →"
            : "Drag the view · ← →",
        ],
        ["Summon or dismiss your shelter", "H · outdoors"],
        ["Use a door or read a field sign", "E · beside it"],
        ["Open or close a door", "F · beside it"],
        ["Open a pane or adjust its blind", "E · pane / J · blind"],
        ["Return to the hearth room", "R"],
        ["Camera / take photograph", "C / P"],
        ["Season and weather", "Click the sky control or press V"],
        ["Sound / close panel", "M / Esc"],
      ];
  for (const pair of entries) {
    const row = el("div", null, "guide-row");
    row.append(el("span", pair[0]), el("span", pair[1]));
    content.append(row);
  }
  content.append(
    el(
      "p",
      "Open a rear or right-hand door to discover a room. The front door retraces your route. Your rooms, windows and blinds are saved on this device.",
      "note",
    ),
  );
  const sightings = Module._mountain_sightings();
  const animals = Array.from({ length: 5 }, (_, k) =>
    sightings & (1 << k)
      ? Module.ccall("mountain_species", "string", ["number"], [k])
      : "",
  ).filter(Boolean);
  const wildlife = el("div", null, "field");
  wildlife.append(
    el("span", "Wildlife nearby", "field-title"),
    el(
      "p",
      animals.length
        ? animals.join(" · ")
        : "Look along forest edges and the open slopes.",
      "note",
    ),
  );
  if (val(36) <= 1) content.append(wildlife);
  if (window.mountainXR?.supported)
    content.append(
      button(
        "Enter VR",
        "action primary",
        () => {
          closeSheet();
          window.mountainXR.start();
        },
        "vr",
      ),
    );
  const foot = el("div", null, "guide-foot");
  foot.innerHTML =
    '<a href="assets/CREDITS.txt" target="_blank" rel="noopener">Art &amp; sound credits</a> · <a href="LICENSE" target="_blank" rel="noopener">Code license</a>';
  content.append(foot);
}
async function openArchive() {
  if (!window.indexedDB) return;
  try {
    const db = await new Promise((ok, fail) => {
      const r = indexedDB.open("mountain-field-journal", 1);
      r.onupgradeneeded = () =>
        r.result.createObjectStore("photos", { keyPath: "id" });
      r.onsuccess = () => ok(r.result);
      r.onerror = () => fail(r.error);
    });
    state.db = db;
    const stored = await new Promise((ok, fail) => {
      const r = db.transaction("photos").objectStore("photos").getAll();
      r.onsuccess = () => ok(r.result);
      r.onerror = () => fail(r.error);
    });
    state.photos = stored
      .concat(state.photos.filter((p) => !stored.some((q) => q.id === p.id)))
      .sort((a, b) => b.id - a.id);
    updateCount();
  } catch {
    state.db = null;
  }
}
function updateCount() {
  $("count").textContent = state.photos.length;
}
function photoURL(p) {
  if (!p.url) p.url = URL.createObjectURL(p.blob);
  return p.url;
}
async function capture() {
  if (!state.ready) return;
  Module._mountain_frame();
  const shot = {
    id: Date.now(),
    trail: nameOf(0),
    room: val(11) ? nameOf(3) : "",
    meal: val(49) ? trayNames() : "",
    seed: val(7),
    season: nameOf(1),
    weather: nameOf(2),
    time: timeText(val(4)),
  };
  const blob = await new Promise((resolve) =>
    canvas.toBlob(resolve, "image/png"),
  );
  if (!blob) {
    notice("The photograph could not be captured.");
    return;
  }
  shot.blob = blob;
  const setting = Module._mountain_meal_photo();
  while (state.photos.some((p) => p.id === shot.id)) shot.id++;
  state.photos.unshift(shot);
  updateCount();
  $("flash").classList.remove("fire");
  void $("flash").offsetWidth;
  $("flash").classList.add("fire");
  try {
    if (!state.db) throw Error();
    await new Promise((ok, fail) => {
      const tx = state.db.transaction("photos", "readwrite");
      tx.objectStore("photos").put(shot);
      tx.oncomplete = ok;
      tx.onerror = fail;
      tx.onabort = fail;
    });
    notice(
      setting
        ? "House dish photographed. A new table setting is yours."
        : "Photograph saved to your journal.",
    );
  } catch {
    notice("Photograph available in this tab. Download it from the journal.");
  }
}
function showPhoto(p) {
  openSheet(p.room || p.trail, p.season + " · " + p.time, true);
  if (p.meal) content.append(el("p", p.meal, "field-title"));
  const img = el("img", null, "large-photo");
  img.src = photoURL(p);
  img.alt =
    "Photograph of " +
    (p.room || p.trail) +
    " under " +
    p.weather.toLowerCase() +
    " skies.";
  content.append(img);
  const row = el("div", null, "photo-tools");
  row.append(button("All photographs", "plain", journal, "back"));
  row.append(
    button(
      "Download PNG",
      "action primary",
      () => {
        const link = document.createElement("a");
        link.href = photoURL(p);
        link.download = "Mountain-" + p.seed + "-" + p.id + ".png";
        link.click();
      },
      "download",
    ),
  );
  content.append(row);
}
function journal() {
  if (
    !openSheet(
      "Photo journal",
      state.photos.length +
        " " +
        (state.photos.length === 1 ? "photograph" : "photographs"),
      true,
    )
  )
    return;
  $("explore").classList.remove("selected");
  $("journal").classList.add("selected");
  if (!state.photos.length) {
    const box = el("div", null, "empty");
    box.innerHTML = icon("journal");
    box.append(
      el("p", "Your trail photographs will appear here."),
      button(
        "Open camera",
        "action primary",
        () => {
          closeSheet();
          mode(true);
        },
        "camera",
      ),
    );
    content.append(box);
    return;
  }
  const grid = el("div", null, "prints");
  content.append(grid);
  for (const p of state.photos) {
    const b = el("button", null, "print"),
      img = el("img", null);
    img.src = photoURL(p);
    img.alt = "";
    b.append(
      img,
      el("strong", p.room || p.trail),
      el("small", p.season + " · " + p.time),
    );
    b.onclick = () => showPhoto(p);
    grid.append(b);
  }
}
$("map").onclick = trailMap;
$("guide").onclick = guide;
$("conditions").onclick = conditions;
$("change-season").onclick = conditions;
$("journal").onclick = journal;
$("brand").onclick = (e) => {
  e.preventDefault();
  closeSheet();
  mode(false);
};
$("explore").onclick = () => {
  closeSheet();
  mode(false);
};
$("close").onclick = closeSheet;
$("camera").onclick = () => mode(true);
$("leave-camera").onclick = () => mode(false);
$("shutter").onclick = capture;
$("cabin").onclick = cabin;
$("use").onclick = () => act(2);
$("cookbook").onclick = () => openCookbook();
$("sit").onclick = () => mealAction(17);
$("sip").onclick = () => mealAction(18);
$("bite").onclick = () => mealAction(19);
$("door").onclick = () => act(9);
$("blind").onclick = () => act(10);
$("sound").onclick = () => {
  if (state.ready) {
    Module._mountain_sound();
    refresh();
  }
};
$("grid").onclick = () => {
  const on = $("thirds").hidden;
  $("thirds").hidden = !on;
  $("grid").setAttribute("aria-pressed", String(on));
};
$("full").onclick = async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await document.documentElement.requestFullscreen();
  } catch {
    notice("Fullscreen is unavailable in this window.");
  }
};
document.addEventListener("fullscreenchange", () => {
  $("full").setAttribute(
    "aria-label",
    document.fullscreenElement ? "Leave fullscreen" : "Enter fullscreen",
  );
  resize();
});
window.addEventListener("keydown", (e) => {
  if (
    !state.ready ||
    sheet.open ||
    e.metaKey ||
    e.altKey ||
    e.ctrlKey ||
    e.target.matches("input,textarea,select")
  )
    return;
  const k = e.key.toLowerCase();
  if (
    [
      "w",
      "a",
      "s",
      "d",
      "arrowup",
      "arrowdown",
      "arrowleft",
      "arrowright",
    ].includes(k)
  ) {
    e.preventDefault();
    state.keys.add(k);
    walk();
    return;
  }
  if (e.repeat) return;
  const actions = {
    q: openCookbook,
    z: () => mealAction(17),
    1: () => mealAction(18),
    2: () => mealAction(19),
    h: () => act(1),
    e: () => act(2),
    f: () => act(9),
    j: () => act(10),
    k: () => act(11),
    l: () => act(12),
    x: () => act(13),
    r: () => act(6),
    c: () => mode(!state.camera),
    p: capture,
    m: () => $("sound").click(),
    g: guide,
    "[": () => visit((val(28) + val(29) - 1) % val(29)),
    "]": () => visit((val(28) + 1) % val(29)),
    v: conditions,
    y: () => act(4),
    u: () => act(5),
    t: () => act(3),
    escape: () => {
      if (!releaseMouse()) mode(false);
    },
  };
  if (actions[k]) {
    e.preventDefault();
    actions[k]();
  }
});
window.addEventListener("keyup", (e) => {
  state.keys.delete(e.key.toLowerCase());
  walk();
});
window.addEventListener("blur", suspendInput);
document.addEventListener("visibilitychange", () => {
  if (document.hidden) suspendInput();
});
function dragSurface(node, start, move, end) {
  let active = null,
    last = null;
  node.addEventListener("pointerdown", (e) => {
    if (active !== null || !state.ready || document.pointerLockElement) return;
    e.preventDefault();
    active = e.pointerId;
    last = [e.clientX, e.clientY];
    node.setPointerCapture(active);
    start(e);
  });
  node.addEventListener("pointermove", (e) => {
    if (e.pointerId !== active) return;
    move(e, e.clientX - last[0], e.clientY - last[1]);
    last = [e.clientX, e.clientY];
  });
  const release = (e) => {
    if (active === e.pointerId) {
      active = null;
      end(e);
    }
  };
  for (const type of ["pointerup", "pointercancel", "lostpointercapture"])
    node.addEventListener(type, release);
}
function lookBy(dx, dy) {
  if (canLook()) Module._mountain_look(dx * 0.004, -dy * 0.004);
}
const lookHint = $("look-hint");
const freeLookHint = canLockMouse ? "Click to look" : "Drag to look";
if (lookHint) lookHint.textContent = freeLookHint;
document.addEventListener("pointerlockchange", () => {
  if (mouseLocked() && !canLook()) {
    releaseMouse();
    return;
  }
  if (lookHint)
    lookHint.textContent = mouseLocked()
      ? "Esc to release mouse"
      : freeLookHint;
});
document.addEventListener("mousemove", (e) => {
  if (mouseLocked()) lookBy(e.movementX, e.movementY);
});
let dragDistance = 0;
dragSurface(
  canvas,
  () => {
    dragDistance = 0;
    canvas.focus({ preventScroll: true });
  },
  (e, dx, dy) => {
    dragDistance += Math.abs(dx) + Math.abs(dy);
    if (!mouseLocked()) lookBy(dx, dy);
  },
  async (e) => {
    // Request pointer lock on mouse clicks and keep drag gestures available.
    if (
      !canLockMouse ||
      e.type !== "pointerup" ||
      e.pointerType !== "mouse" ||
      e.button !== 0 ||
      dragDistance > 4 ||
      !canLook() ||
      document.pointerLockElement
    )
      return;
    try {
      await canvas.requestPointerLock();
    } catch {
      // Drag and arrow controls are available if the browser declines capture.
    }
  },
);
const pad = $("pad");
function stick(e) {
  const r = pad.getBoundingClientRect(),
    dx = e.clientX - r.left - r.width / 2,
    dy = e.clientY - r.top - r.height / 2;
  const radius = r.width * 0.36,
    scale = 1 / Math.max(radius, Math.hypot(dx, dy));
  state.stick = [-dy * scale, dx * scale];
  pad.firstElementChild.style.transform =
    "translate(" +
    dx * scale * radius * 0.65 +
    "px," +
    dy * scale * radius * 0.65 +
    "px)";
  walk();
}
dragSurface(pad, stick, stick, () => {
  state.stick = [0, 0];
  pad.firstElementChild.style.transform = "";
  walk();
});
let lastTurn = 0;
function keyboardLook(now) {
  const dt = Math.min(0.12, (now - lastTurn) / 1000);
  lastTurn = now;
  if (state.ready && !sheet.open) {
    const yaw =
      Number(state.keys.has("arrowright")) -
      Number(state.keys.has("arrowleft"));
    if (yaw) Module._mountain_look(yaw * dt * 1.45, 0);
  }
  requestAnimationFrame(keyboardLook);
}
window.mountainReady = () => {
  state.ready = true;
  resize();
  refresh();
  $("loading").hidden = true;
  setInterval(refresh, 240);
  requestAnimationFrame(keyboardLook);
  window.mountainXR?.probe();
};
window.mountainError = (message) => {
  $("loading").hidden = false;
  $("loading").querySelector("p").textContent =
    message || "The trail could not open.";
};
window.addEventListener("resize", resize);
new ResizeObserver(resize).observe($("viewport"));
openArchive();
