"use strict";
const cabinGuideAction = 33;
const wallNames = ["Entry wall", "Far wall", "Right wall", "Balcony"];
const roomField = (id, field) => Module._mountain_room(id, field);
const roomName = (id) =>
  Module.ccall("mountain_room_name", "string", ["number"], [id]);
const cabinPose = (field) => Module._mountain_cabin_pose(field);

function hasCabinMap() {
  return typeof Module._mountain_room_door === "function";
}

function updateCabinMapControls(inside, interaction) {
  const title = $("map").querySelector("span");
  title.textContent = inside && hasCabinMap() ? "Cabin map" : "Trail map";
  const way = $("cabin-way");
  if (!way) return;
  const target = inside ? nameOf(23) : "";
  way.hidden = !target || state.camera || (val(12) !== 0 && !val(62));
  if (!way.hidden && hasCabinMap()) {
    $("cabin-direction").style.transform =
      "rotate(" + (cabinPose(4) - cabinPose(2)) + "rad)";
    $("cabin-destination").textContent =
      target === "Balcony" ? "Balcony door" : "Balcony via " + target;
  }
  if (inside && [4, 5, 8].includes(interaction)) {
    const room = nameOf(22);
    if (room) {
      const action = interaction === 8 ? "Open door to " : "Enter ";
      $("use").querySelector("span").textContent =
        action + room.replace(/^The /, "the ");
    }
  }
}

function cabinRoom(id) {
  const links = [7, 8, 9].map((field) => roomField(id, field));
  return {
    id,
    name: roomName(id),
    width: roomField(id, 2),
    depth: roomField(id, 3),
    height: roomField(id, 4),
    balcony: Boolean(roomField(id, 5)),
    doors: roomField(id, 6),
    links,
  };
}

function discoveredRooms() {
  const rooms = [],
    pending = [0];
  while (pending.length) {
    const id = pending.pop();
    rooms.push({
      id,
      name: roomName(id),
      level: roomField(id, 1),
      balcony: Boolean(roomField(id, 5)),
    });
    const right = roomField(id, 9),
      rear = roomField(id, 8);
    if (right >= 0) pending.push(right);
    if (rear >= 0) pending.push(rear);
  }
  return rooms;
}

function drawCabinPlan(canvas, room, position, route) {
  const ctx = canvas.getContext("2d"),
    w = canvas.width,
    h = canvas.height;
  const balconyWidth = room.balcony ? 3.2 : 0;
  const scale = Math.min(
    (w - 110) / (room.width + balconyWidth),
    (h - 74) / room.depth,
  );
  const x = (w + balconyWidth * scale) / 2,
    y = h / 2;
  const left = x - (room.width * scale) / 2,
    right = x + (room.width * scale) / 2;
  const top = y - (room.depth * scale) / 2,
    bottom = y + (room.depth * scale) / 2;
  const deck = balconyWidth * scale,
    deckDepth = (bottom - top) * 0.6;

  ctx.fillStyle = "#ebe8d8";
  ctx.fillRect(0, 0, w, h);
  ctx.strokeStyle = "#dedbca";
  ctx.lineWidth = 1;
  ctx.beginPath();
  for (let i = 12; i < w; i += 16) {
    ctx.moveTo(i, 0);
    ctx.lineTo(i, h);
  }
  for (let j = 12; j < h; j += 16) {
    ctx.moveTo(0, j);
    ctx.lineTo(w, j);
  }
  ctx.stroke();
  if (room.balcony) {
    ctx.fillStyle = "#bed1bd";
    ctx.fillRect(left - deck, y - deckDepth / 2, deck, deckDepth);
    ctx.strokeStyle = "#698c79";
    ctx.strokeRect(left - deck, y - deckDepth / 2, deck, deckDepth);
    ctx.beginPath();
    for (let i = left - 7; i > left - deck; i -= 7) {
      ctx.moveTo(i, y - deckDepth / 2);
      ctx.lineTo(i, y + deckDepth / 2);
    }
    ctx.stroke();
  }
  ctx.fillStyle = "#fcf7e8";
  ctx.fillRect(left, top, right - left, bottom - top);
  ctx.strokeStyle = "#354c41";
  ctx.lineWidth = 3;
  ctx.strokeRect(left, top, right - left, bottom - top);
  const centers = [
    [x, bottom],
    [x, top],
    [right, y],
    [left, y],
  ];
  const span = Math.max(15, 1.75 * scale);
  for (let wall = 0; wall < 4; wall++) {
    const [dx, dy] = centers[wall],
      vertical = wall > 1;
    const isOpen = (room.doors & (1 << wall)) !== 0;
    if (wall !== 3 || room.balcony) {
      ctx.fillStyle =
        route === wall ? "#dcac6d" : isOpen ? "#7fa28d" : "#a68868";
      ctx.fillRect(
        dx - (vertical ? 4 : span / 2),
        dy - (vertical ? span / 2 : 4),
        vertical ? 8 : span,
        vertical ? span : 8,
      );
    }
    const length = (vertical ? bottom - top : right - left) * 0.084;
    ctx.strokeStyle = "#75a69c";
    ctx.lineWidth = 3;
    for (const sign of [-1, 1]) {
      const shift = (sign * 0.6 * (vertical ? bottom - top : right - left)) / 2;
      ctx.beginPath();
      ctx.moveTo(
        vertical ? dx : dx + shift - length,
        vertical ? dy + shift - length : dy,
      );
      ctx.lineTo(
        vertical ? dx : dx + shift + length,
        vertical ? dy + shift + length : dy,
      );
      ctx.stroke();
    }
  }
  ctx.fillStyle = "#465b4b";
  ctx.font = "600 13px 'DM Sans', sans-serif";
  ctx.textAlign = "center";
  ctx.fillText("Far door", x, top - 13);
  ctx.fillText(room.id ? "Entry door" : "Trail", x, bottom + 22);
  ctx.save();
  ctx.translate(right + 17, y);
  ctx.rotate(Math.PI / 2);
  ctx.fillText("Right door", 0, 0);
  ctx.restore();
  if (room.balcony) {
    ctx.save();
    ctx.translate(left - deck - 11, y);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText("Balcony", 0, 0);
    ctx.restore();
  }
  if (position) {
    ctx.save();
    ctx.translate(x + position.x * scale, y + position.z * scale);
    ctx.rotate(position.yaw);
    ctx.fillStyle = "#9f4a35";
    ctx.strokeStyle = "#fff9ec";
    ctx.lineWidth = 2.25;
    ctx.beginPath();
    ctx.moveTo(0, -12);
    ctx.lineTo(8, 8);
    ctx.lineTo(0, 5);
    ctx.lineTo(-8, 8);
    ctx.closePath();
    ctx.fill();
    ctx.stroke();
    ctx.restore();
  }
}

function cabinMap() {
  if (!val(11) || !hasCabinMap() || !openSheet("Cabin map", nameOf(12), true))
    return;
  const here = val(12),
    onDeck = Boolean(val(50)),
    route = cabinPose(3);
  const position = { x: cabinPose(0), z: cabinPose(1), yaw: cabinPose(2) };
  const rooms = discoveredRooms();
  let selected = here,
    listPage = 0;
  const tools = el("div", null, "cabin-map-actions");
  const current = button(
    "You are here",
    "action",
    () => show(here, true),
    "compass",
  );
  const hearth = button("Hearth Room", "action", () => show(0, true), "cabin");
  const follow = button(
    val(62) ? "Clear balcony route" : "Follow balcony route",
    "action primary",
    () => {
      act(cabinGuideAction);
      closeSheet();
    },
  );
  tools.append(current, hearth, follow, button("Trail map", "plain", trailMap));
  if (val(29)) tools.append(button("Choose a destination", "plain", atlas));
  const grid = el("div", null, "cabin-map-grid");
  const plan = el("section", null, "cabin-map-plan"),
    index = el("section", null, "cabin-index");
  grid.append(plan, index);
  content.append(tools, grid);

  const count =
    rooms.length + (rooms.length === 1 ? " mapped room" : " mapped rooms");
  const search = el("input");
  search.type = "search";
  search.placeholder = "Find a mapped room";
  search.setAttribute("aria-label", "Find a mapped room");
  const results = el("div", null, "cabin-index-list");
  index.append(el("h3", count), search, results);
  search.addEventListener("input", () => {
    listPage = 0;
    drawIndex();
  });

  function show(id, focus = false) {
    selected = id;
    const room = cabinRoom(id),
      isHere = id === here;
    const deck = isHere && onDeck;
    const dimensions =
      [room.width, room.depth, room.height]
        .map((x) => Math.round(x * 10) / 10)
        .join(" × ") + " m";
    const map = el("canvas", null, "cabin-plan-frame");
    map.width = 540;
    map.height = 290;
    map.setAttribute(
      "aria-label",
      "Floor plan of " +
        room.name +
        (isHere ? " showing your position and heading" : ""),
    );
    drawCabinPlan(map, room, isHere ? position : null, isHere ? route : -1);
    const header = el("div", null, "cabin-plan-head");
    const name = el("h3", room.name);
    name.tabIndex = -1;
    header.append(
      name,
      el(
        "p",
        (isHere
          ? (deck ? "You are on the balcony" : "You are here") + " · "
          : "") + dimensions,
      ),
    );
    const doors = el("div", null, "cabin-door-grid");
    for (let wall = 0; wall < (room.balcony ? 4 : 3); wall++) {
      const neighbor = room.links[wall] ?? -1;
      const text =
        Module.ccall(
          "mountain_room_door",
          "string",
          ["number", "number"],
          [id, wall],
        ) || "Unexplored room";
      const open = Boolean(room.doors & (1 << wall));
      const status = open
        ? "Open"
        : neighbor < 0 && wall > 0 && wall < 3
          ? "Unopened"
          : "Closed";
      const direction = el("small", wallNames[wall] + " · " + status);
      const label = el("strong", text);
      const cell =
        neighbor >= 0
          ? el("button", null, "cabin-door-card")
          : el("div", null, "cabin-door-card");
      cell.append(direction, label);
      if (neighbor >= 0) cell.onclick = () => show(neighbor, true);
      if (isHere && route === wall) {
        cell.classList.add("balcony-direction");
        cell.append(
          el(
            "small",
            wall === 3 ? "Step onto the balcony" : "Balcony this way",
          ),
        );
      }
      doors.append(cell);
    }
    plan.replaceChildren(header, map, doors);
    current.disabled = selected === here;
    hearth.disabled = selected === 0;
    drawIndex();
    if (focus) name.focus();
  }

  function drawIndex() {
    const query = search.value.trim().toLowerCase();
    const matches = rooms.filter(
      (r) =>
        r.name.toLowerCase().includes(query) ||
        (r.balcony && "balcony".includes(query)),
    );
    const pageSize = 8,
      pages = Math.ceil(matches.length / pageSize);
    listPage = Math.min(listPage, Math.max(0, pages - 1));
    results.replaceChildren();
    for (const r of matches.slice(
      listPage * pageSize,
      (listPage + 1) * pageSize,
    )) {
      const item = el("button", null, "cabin-index-room");
      item.setAttribute("aria-pressed", String(selected === r.id));
      item.append(
        el("strong", r.name),
        el(
          "small",
          r.id
            ? "Room " +
                (r.id + 1) +
                " · " +
                r.level +
                (r.level === 1 ? " doorway" : " doorways") +
                " from the Hearth"
            : "Entrance",
        ),
      );
      if (r.id === here)
        item.append(el("span", "You are here", "cabin-place-tag"));
      if (r.balcony) item.append(el("small", "Balcony"));
      item.onclick = () => show(r.id, true);
      results.append(item);
    }
    if (!matches.length)
      results.append(el("p", "No rooms match your search.", "note"));
    if (pages > 1) {
      const nav = el("div", null, "cabin-index-pages");
      const prev = button("Previous", "plain", () => {
        listPage--;
        drawIndex();
      });
      const next = button("Next", "plain", () => {
        listPage++;
        drawIndex();
      });
      prev.disabled = listPage === 0;
      next.disabled = listPage + 1 === pages;
      nav.append(prev, el("span", listPage + 1 + " / " + pages), next);
      results.append(nav);
    }
  }

  show(here);
}
