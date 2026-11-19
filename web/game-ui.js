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
const val = (id) => Module._mountain_value(id);
const nameOf = (id) =>
  Module.ccall("mountain_label", "string", ["number"], [id]);
const icon = (name) => '<i aria-hidden="true" class="icon i-' + name + '"></i>';
const near = [
  "",
  "Open door",
  "Step inside",
  "Step outside",
  "Next room",
  "Previous room",
];
function notice(message) {
  clearTimeout(state.toastTimer);
  $("toast").textContent = message;
  $("toast").classList.add("shown");
  state.toastTimer = setTimeout(
    () => $("toast").classList.remove("shown"),
    3100,
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
    ? "Cabin · Room " + (val(12) + 1)
    : val(25) && val(7) === val(23)
      ? nameOf(5) + " · Alpine trail"
      : "Alpine trail";
  $("height").textContent = Math.round(val(8)).toLocaleString() + " m";
  $("season").textContent = nameOf(1);
  $("clock").textContent = timeText(h);
  $("skies").textContent = nameOf(2);
  $("sky-icon").className =
    "icon i-" +
    (weather
      ? ["sun", "cloud", "rain", "mist"][weather]
      : h < 6 || h >= 19
        ? "moon"
        : "sun");
  $("use").hidden = !use || state.camera;
  $("use").querySelector("span").textContent = use;
  $("cabin").querySelector("span").textContent = inside
    ? "Hearth room"
    : val(9)
      ? "Dismiss cabin"
      : "Summon cabin";
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
function act(action) {
  if (!state.ready) return;
  const result = Module._mountain_action(action);
  if (action === 1 && result)
    notice(val(9) ? "Your cabin is here." : "Cabin dismissed.");
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
  stop();
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
  if (state.ready) Module._mountain_pause(0);
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
    weatherGroup.children[2].textContent = options(1, 4)[2];
  });
  const cycle = modeControl("Cycle through seasons", 18, 7);
  choose(content, "Weather", options(1, 4), val(6), (i) => {
    Module._mountain_select(1, i);
    changing.checked = false;
  });
  const weatherGroup = content.querySelector(
    '[role="group"][aria-label="Weather"]',
  );
  const changing = modeControl("Let the weather change", 19, 8);
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
}
function topography(map) {
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
  const coordinates = [...trail, hiker],
    xs = coordinates.map((p) => p[0]),
    zs = coordinates.map((p) => p[1]);
  const minX = Math.min(...xs),
    maxX = Math.max(...xs),
    minZ = Math.min(...zs),
    maxZ = Math.max(...zs);
  const step = Math.max(2.75, (maxX - minX + 56) / nx, (maxZ - minZ + 52) / ny);
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
  ctx.beginPath();
  trail
    .map(project)
    .forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
  ctx.lineWidth = 5.4;
  ctx.strokeStyle = "#fff4d8";
  ctx.stroke();
  ctx.lineWidth = 2.7;
  ctx.strokeStyle = "#a14e33";
  ctx.stroke();
  for (const [index, label] of [
    [val(21), "1"],
    [trail.length - 1, "2"],
  ]) {
    const [x, y] = project(trail[index]);
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
    "Contour map showing your position and the marked trail to Lakeside View and Ridge Lookout",
  );
  content.append(map);
  topography(map);
  content.append(el("p", "① Lakeside View　 ② Ridge Lookout", "route-legend"));
  const details = el("div", null, "map-details");
  details.append(
    el("span", Math.round(val(22) * val(24)).toLocaleString() + " m trail"),
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
  const start = (seed) => {
    Module._mountain_seed(seed);
    mode(false);
    closeSheet();
    refresh();
    notice("Arrived at " + nameOf(0) + ".");
  };
  form.onsubmit = (e) => {
    e.preventDefault();
    if (form.reportValidity()) start(Number(input.value));
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
      "Generate a new mountain",
      "action primary",
      () => start(crypto.getRandomValues(new Uint32Array(1))[0]),
      "shuffle",
    ),
  );
}
function guide() {
  if (!openSheet("Field guide", "Trail controls")) return;
  const touch = matchMedia("(pointer:coarse)").matches;
  const entries = touch
    ? [
        ["Walk", "Move the thumb pad"],
        ["Season and weather", "Tap the sky control"],
        ["Look around", "Drag the landscape"],
        ["Call your cabin", "Summon cabin in the trail kit"],
        ["Enter or explore rooms", "Tap the door prompt"],
        ["Photograph a view", "Camera → Take photo"],
      ]
    : [
        ["Walk", "W A S D · ↑ ↓"],
        ["Look around", "Drag the view · ← →"],
        ["Summon or dismiss cabin", "H · outdoors"],
        ["Open a door or cross it", "E · near a doorway"],
        ["Return to the hearth room", "R · indoors"],
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
      "Doors at the far end lead to another room. The front door in the Hearth Room leads back to the trail.",
      "note",
    ),
  );
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
    notice("Photograph saved to your journal.");
  } catch {
    notice("Photograph available in this tab. Download it from the journal.");
  }
}
function showPhoto(p) {
  openSheet(p.room || p.trail, p.season + " · " + p.time, true);
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
    h: () => act(1),
    e: () => act(2),
    r: () => act(6),
    c: () => mode(!state.camera),
    p: capture,
    m: () => $("sound").click(),
    g: guide,
    v: conditions,
    y: () => act(4),
    u: () => act(5),
    t: () => act(3),
    escape: () => mode(false),
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
window.addEventListener("blur", stop);
document.addEventListener("visibilitychange", () => {
  if (document.hidden) stop();
});
function dragSurface(node, start, move, end) {
  let active = null,
    last = null;
  node.addEventListener("pointerdown", (e) => {
    if (active !== null || !state.ready) return;
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
      end();
    }
  };
  for (const type of ["pointerup", "pointercancel", "lostpointercapture"])
    node.addEventListener(type, release);
}
dragSurface(
  canvas,
  () => canvas.focus({ preventScroll: true }),
  (e, dx, dy) => Module._mountain_look(dx * 0.004, -dy * 0.004),
  () => {},
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
