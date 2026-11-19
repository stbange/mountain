// Fetch individually addressable assets into Lua's and SDL's shared filesystem.
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
  addRunDependency(" mountain-assets");
  (async function () {
    const list = await fetch("assets.json");
    if (!list.ok) throw Error("Could not open the asset list.");
    const paths = await list.json();
    await Promise.all(
      paths.map(async function (path) {
        const res = await fetch(path);
        if (!res.ok) throw Error("Could not load " + path);
        const bytes = new Uint8Array(await res.arrayBuffer());
        const parent = path.slice(0, path.lastIndexOf("/"));
        FS.mkdirTree("/" + parent);
        FS.writeFile("/" + path, bytes);
      }),
    );
    removeRunDependency(" mountain-assets");
  })().catch(function (error) {
    if (window.mountainError) window.mountainError(error.message);
    else console.error(error);
  });
});
