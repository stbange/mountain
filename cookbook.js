"use strict";
const bookView = {
  recipes: null,
  filter: "Local",
  search: "",
  page: 0,
  selected: 0,
  tab: "Recipes",
};
const recipeText = (id, field) =>
  Module.ccall("mountain_recipe", "string", ["number", "number"], [id, field]);
const itemText = (id, field) =>
  Module.ccall(
    "mountain_ingredient",
    "string",
    ["number", "number"],
    [id, field],
  );
const recipeValue = (id, field) => Module._mountain_recipe_value(id, field);
const stock = (id, field = 0) => Module._mountain_stock(id, field);
window.addEventListener("pagehide", () => {
  if (state.ready) Module._mountain_flush_pantry();
});
function mealAction(action) {
  const ok = Module._mountain_action(action);
  if (nameOf(17)) notice(nameOf(17));
  refresh();
  return ok;
}
function dishList() {
  if (!bookView.recipes) {
    bookView.recipes = [];
    for (let id = 1; id <= val(44); id++) {
      const name = recipeText(id, 0);
      if (name)
        bookView.recipes.push({
          id,
          name,
          method: recipeText(id, 1),
          region: recipeValue(id, 3),
          drink: recipeValue(id, 4),
        });
    }
  }
  return bookView.recipes;
}
function kitchenButton(label, handler, primary = false) {
  const b = button(label, "action" + (primary ? " primary" : ""), handler);
  b.type = "button";
  return b;
}
function trayNames() {
  return [nameOf(20), nameOf(21)].filter(Boolean).join(" and ");
}
function cookDish(id) {
  const ok = Module._mountain_prepare(id);
  notice(nameOf(17));
  return ok;
}
function openCookbook() {
  if (!state.ready) return;
  if (!val(51)) Module._mountain_action(14);
  if (!bookView.selected) bookView.selected = val(53);
  if (!openSheet("Field cookbook", nameOf(12), true)) return;
  drawKitchen();
}
function drawKitchen() {
  content.replaceChildren();
  const hearth = Boolean(val(11) && val(12) === 0);
  const hero = el("div", null, "cook-hero");
  const im = el("img");
  im.src = "ui/atlas/" + val(28) + ".webp";
  im.alt = "";
  const copy = el("div");
  copy.append(
    el(
      "h3",
      hearth ? "Hearth Room kitchen" : val(50) ? "Balcony table" : nameOf(0),
    ),
  );
  const tray = trayNames();
  copy.append(el("p", tray ? "On your tray · " + tray : "Your tray is empty."));
  hero.append(copy, im);
  content.append(hero);
  const quick = el("div", null, "cook-actions");
  if (hearth) {
    quick.append(
      kitchenButton(
        "Brew coffee",
        () => {
          cookDish(val(53));
          drawKitchen();
        },
        true,
      ),
      kitchenButton("Bake oat cookies", () => {
        cookDish(val(54));
        drawKitchen();
      }),
    );
    if (tray)
      quick.append(
        kitchenButton("Carry tray to balcony", () => {
          if (mealAction(20)) {
            closeSheet();
            notice("Your table is on the balcony. Select Sit down.");
          }
        }),
      );
  } else if (val(50)) {
    quick.append(
      kitchenButton(
        val(49) ? "Stand up" : "Sit down",
        () => {
          mealAction(17);
          closeSheet();
        },
        true,
      ),
    );
  } else {
    quick.append(
      kitchenButton("Go to Hearth Room kitchen", () => {
        if (Module._mountain_action(6)) {
          Module._mountain_action(14);
          refresh();
          drawKitchen();
        } else notice("Return to steady ground before calling the kitchen.");
      }),
    );
  }
  if (val(55))
    quick.append(
      kitchenButton("Grow a pinned ingredient", () => {
        mealAction(24);
        drawKitchen();
      }),
    );
  content.append(quick);
  const tabs = el("div", null, "cook-tabs");
  for (const name of ["Recipes", "Basket", "Experiment"]) {
    const b = button(name, "choice", () => {
      bookView.tab = name;
      drawKitchen();
    });
    b.setAttribute("aria-pressed", String(bookView.tab === name));
    tabs.append(b);
  }
  content.append(tabs);
  if (bookView.tab === "Basket") {
    basketPage();
    return;
  }
  if (bookView.tab === "Experiment") {
    experimentPage(hearth);
    return;
  }
  const tools = el("div", null, "cook-search"),
    search = el("input"),
    filter = el("select");
  search.type = "search";
  search.placeholder = "Find a recipe";
  search.setAttribute("aria-label", "Find a recipe");
  search.value = bookView.search;
  filter.setAttribute("aria-label", "Recipe shelf");
  for (const name of [
    "Local",
    "Ready",
    "Drinks",
    "Cross-world",
    "Discovered",
    "All",
  ]) {
    const o = el("option", name);
    filter.append(o);
  }
  filter.value = bookView.filter;
  tools.append(search, filter);
  content.append(tools);
  const body = el("div", null, "cook-body");
  content.append(body);
  function list() {
    body.replaceChildren();
    const query = bookView.search.trim().toLowerCase();
    const recipes = dishList().filter(
      (r) =>
        r.name.toLowerCase().includes(query) &&
        (bookView.filter === "All" ||
          (bookView.filter === "Local" && r.region === val(36)) ||
          (bookView.filter === "Ready" && recipeValue(r.id, 0)) ||
          (bookView.filter === "Drinks" && r.drink) ||
          (bookView.filter === "Cross-world" && r.region < 0) ||
          (bookView.filter === "Discovered" && recipeValue(r.id, 1))),
    );
    if (!recipes.length) {
      body.append(el("p", "No matching recipes.", "note"));
      return;
    }
    const pages = Math.ceil(recipes.length / 6);
    bookView.page = Math.min(bookView.page, pages - 1);
    const page = recipes.slice(bookView.page * 6, bookView.page * 6 + 6);
    if (!page.some((r) => r.id === bookView.selected))
      bookView.selected = page[0].id;
    const left = el("div", null, "dish-list");
    for (const r of page) {
      const b = button(r.name, "dish-row", () => {
        bookView.selected = r.id;
        list();
      });
      b.setAttribute("aria-pressed", String(r.id === bookView.selected));
      left.append(b);
    }
    const pager = el("div", null, "cook-pages");
    const prev = button("Previous", "plain", () => {
      bookView.page--;
      list();
    });
    const next = button("Next", "plain", () => {
      bookView.page++;
      list();
    });
    prev.disabled = bookView.page === 0;
    next.disabled = bookView.page + 1 === pages;
    pager.append(prev, el("span", bookView.page + 1 + " / " + pages), next);
    left.append(pager);
    const card = el("article", null, "recipe-card"),
      id = bookView.selected;
    const r = recipes.find((x) => x.id === id);
    card.append(
      el(
        "span",
        r.method +
          (r.drink ? " · Drink" : "") +
          (id === val(58) ? " · House dish" : ""),
        "field-title",
      ),
      el("h3", r.name),
    );
    const gathered = [];
    for (let k = 5; k <= 7; k++) {
      const item = recipeValue(id, k);
      if (!item) continue;
      const row = el("div", null, "cook-ingredient");
      row.append(
        el("span", itemText(item, 0)),
        el("span", stock(item) + " in pantry"),
      );
      card.append(row);
      gathered.push(item);
    }
    if (recipeText(id, 3))
      card.append(el("p", "Cupboard · " + recipeText(id, 3), "note"));
    const missing = gathered.find((x) => !stock(x));
    if (missing)
      card.append(
        el("p", itemText(missing, 0) + " · " + itemText(missing, 1), "note"),
      );
    const buttons = el("div", null, "cook-actions");
    const cook = kitchenButton(
      r.drink ? "Make drink" : "Cook dish",
      () => {
        cookDish(id);
        drawKitchen();
      },
      true,
    );
    cook.disabled = !hearth || !recipeValue(id, 0);
    const pin = kitchenButton(
      val(46) === id ? "Unpin recipe" : "Pin recipe",
      () => {
        Module._mountain_choose_recipe(id);
        mealAction(16);
        drawKitchen();
      },
    );
    buttons.append(cook, pin);
    if (recipeValue(id, 2))
      buttons.append(
        kitchenButton("Pack a portion", () => {
          Module._mountain_pack(id);
          notice(nameOf(17));
          drawKitchen();
        }),
      );
    card.append(buttons);
    body.append(left, card);
  }
  search.oninput = () => {
    bookView.search = search.value;
    bookView.page = 0;
    list();
  };
  filter.onchange = () => {
    bookView.filter = filter.value;
    bookView.page = 0;
    list();
  };
  list();
}
function basketPage() {
  const pin = val(46);
  if (pin)
    content.append(el("p", "Pinned · " + recipeText(pin, 0), "field-title"));
  if (!val(11)) content.append(el("p", "Nearby · " + nameOf(19), "note"));
  const table = el("div", null, "basket-grid");
  let count = 0;
  for (let i = 1; i <= val(45); i++)
    if (stock(i, 1)) {
      const card = el("div", null, "cook-ingredient");
      card.append(el("span", itemText(i, 0)), el("b", String(stock(i))));
      table.append(card);
      count++;
    }
  content.append(
    count
      ? table
      : el(
          "p",
          "Find a patch along the trail and use Gather to start your basket.",
          "note",
        ),
  );
  if (val(11)) {
    const actions = el("div", null, "cook-actions");
    actions.append(
      kitchenButton("Rest and regrow patches", () => {
        mealAction(25);
        notice("The trail patches have regrown.");
      }),
    );
    if (val(56))
      actions.append(
        kitchenButton("Change tableware", () => {
          mealAction(26);
          notice("Table setting changed.");
        }),
      );
    content.append(actions);
  }
}
function experimentPage(hearth) {
  const form = el("div", null, "cook-experiment");
  const method = el("select");
  method.setAttribute("aria-label", "Cooking method");
  for (const m of [...new Set(dishList().map((r) => r.method))])
    method.append(el("option", m));
  form.append(method);
  const selectors = [];
  for (let slot = 0; slot < 3; slot++) {
    const s = el("select");
    s.setAttribute("aria-label", "Ingredient " + (slot + 1));
    const empty = el(
      "option",
      slot ? "Optional ingredient" : "Choose an ingredient",
    );
    empty.value = "0";
    s.append(empty);
    for (let i = 1; i <= val(45); i++)
      if (stock(i)) {
        const o = el("option", itemText(i, 0));
        o.value = i;
        s.append(o);
      }
    selectors.push(s);
    form.append(s);
  }
  const go = kitchenButton(
    "Try the combination",
    () => {
      const ids = selectors.map((s) => Number(s.value)).filter(Boolean);
      if (!ids.length || new Set(ids).size !== ids.length) {
        notice("Choose one to three different ingredients.");
        return;
      }
      while (ids.length < 3) ids.push(0);
      const ok = Module.ccall(
        "mountain_experiment",
        "number",
        ["string", "number", "number", "number"],
        [method.value, ...ids],
      );
      notice(
        ok
          ? nameOf(17)
          : "This combination needs ingredients from your pantry.",
      );
      if (ok) drawKitchen();
    },
    true,
  );
  go.disabled = !hearth;
  form.append(go);
  content.append(form);
}
