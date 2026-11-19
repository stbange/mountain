# Mountain

A quiet first-person hiking game set on Earth. Walk down from Larkspur Ridge, wait for the light on the lake, and take a photograph. Call a wood cabin onto the trail when you want to come inside.

[Play the browser build](https://raw.githack.com/stbange/mountain/gh-page/index.html) · [Download desktop builds from the latest successful playtest run](https://github.com/stbange/mountain/actions/workflows/playtest.yml)

![An in-game summer photograph of the lake below Larkspur Ridge](docs/larkspur-ridge.png)

## On the trail

The first trail opens at a reproducible overlook. **Trail map** shows contours and your heading. Enter a seed to revisit a mountain, or choose **Generate a new mountain**. The clock advances and the weather changes while hiking. Click the weather and time indicator to choose a season, change the weather, or wait for a particular light.

**Summon cabin** places a furnished lodge ahead. Its footings follow the slope. In a grove, a mature tree at the edge of the floor can carry the platform. Open the front door to see the Hearth Room, then walk through. The door at the far end leads to another room. Rooms are regenerated from their index as you move. Their windows look back onto the current mountain. Walk back the way you came, or use **R** to return to the entrance. Step outside to dismiss the house.

**Camera** opens the viewfinder. Take a photograph, then open **Photo journal** to view it or download its PNG. The browser keeps photographs in IndexedDB on that browser and device. Native photographs are lossless BMPs in the SDL user preference directory for Mountain. The window title and console show the saved path.

| Control | Desktop | Touch web |
| --- | --- | --- |
| Move | W A S D or up/down arrows | Thumb pad |
| Look | Drag the web view or use left/right arrows. On native, click to capture the mouse and Esc to release it. | Drag the landscape |
| Summon or dismiss the cabin outdoors | H | Cabin button |
| Open a door or cross the nearby threshold | E or walk through an open door | Door prompt or thumb pad |
| Return to the Hearth Room | R while indoors | Hearth room button |
| Photograph | P. On web, C opens the viewfinder. | Camera, then Take photo |
| Sound | M | Speaker button |
| Guide | G on web, F1 on native | Field guide |
| Native conditions / a new seed | T for time, Y for season, U for weather, N for a new mountain | Trail map and weather controls |

On an immersive-VR-capable browser, open **Field guide → Enter VR**. Head tracking supplies the views. Use the left stick to hike, the right stick to snap-turn, the trigger to use a door, and squeeze to summon or dismiss outdoors. WebXR requires HTTPS or localhost.

## Build

Use CMake 3.24+, Python 3, a C11 compiler and a desktop OpenGL driver with framebuffer objects. CMake downloads hash-pinned SDL 3.2.24 and Lua 5.4.8. It links them into the application and stages editable content beside the binary. Linux needs the X11 or Wayland development headers, OpenGL/EGL development headers and an audio backend. The Linux CI job lists a working Ubuntu package set.

~~~sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
~~~

Run **build/play/mountain** on Linux or **build/play/mountain.exe** on Windows. Use a Visual Studio developer shell for Ninja on Windows. Multi-configuration generators put the selected configuration under play/. Keep the adjacent assets directory with the executable.

For the web, activate Emscripten 4.0.20 and build the same sources.

~~~sh
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DMOUNTAIN_TESTS=OFF
cmake --build build-web
python3 -m http.server 8000 --directory build-web/play
~~~

Open http://localhost:8000/. Serve the entire directory. The loader reads the manifest and fetches the individual assets relative to the game URL, including when deployed under /mountain/.

## Engine and content

| Location | Responsibility |
| --- | --- |
| src/world.c and src/game.c | Seeded relief, vegetation placement, clock, hiking, thresholds and cabin state |
| src/mesh.c, src/scene.c and src/render.c | Terrain meshes, sprite stands, voxel details, reflective water, window views and OpenGL passes |
| src/script.c | Loads the Lua content and exposes the room geometry builder |
| src/app.c and web/ | SDL input, sound and photos, browser controls, journal and WebXR |
| assets/scripts/world.lua | Starting trail, relief, water, pace, clock rate, names, seasons and palettes |
| assets/scripts/cabin.lua | Floor plans, window openings, furniture, fixtures, materials and room titles |
| assets/art, assets/shaders and assets/sound | Original bitmap art, GLSL and loopable PCM ambience |

Edit the staged Lua or art files and reopen the game to see changes without recompiling. To add a room design, extend build_room in cabin.lua. The builder takes centred boxes in metres, colour, material, yaw and optional roof tilt. Room zero is the main cabin. The renderer holds the active room and its immediate neighbours, so memory usage stays bounded as the hiker explores. Additional terrain types or planetary sky models can extend the C generator and the Lua world definition.

## Specifications and tests

OpenSpec records the accepted behaviour in openspec/specs/. The test suite checks deterministic terrain, climate, cabin collisions, door apertures, reproducible rooms, graphics, input and audio. Coverage uses native GCC/gcov for the project's C sources and Lua debug line hooks for gameplay scripts. Third-party sources are outside both totals. The browser suite drives actual input and framebuffer draws, including an emulated tracked headset.

~~~sh
npx --yes @fission-ai/openspec@2.1.0 validate --all --strict
cmake -S . -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMOUNTAIN_COVERAGE=ON
cmake --build build-test
ctest --test-dir build-test -R '^unit$' --output-on-failure
python3 tools/coverage.py build-test
ctest --test-dir build-test -R '^(lua-content|game)$' -V
python3 -m pip install -r tests/requirements.txt
python3 -m playwright install chromium
python3 tests/browser.py build-web/play
~~~

Use a fresh coverage build to measure the unit run independently. For headless Linux set SDL_VIDEO_DRIVER=offscreen, EGL_PLATFORM=surfaceless, SDL_AUDIO_DRIVER=dummy and LIBGL_ALWAYS_SOFTWARE=1. An existing Chromium executable can be selected with CHROMIUM. The live playtest renders a closed and opened cabin, hikes through successive rooms, returns outside and writes a photograph. Browser tests also download a photo, reload the journal and exercise phone controls.

## Branches and licensing

Development takes place on **main**. **gh-page** contains the staged browser distribution. The playtest workflow builds Linux, Windows and Web, runs their checks and advances gh-page after they pass. The play link above serves that branch through Raw.githack. To use the repository’s own GitHub Pages address, select **Deploy from a branch → gh-page → / (root)** in [Pages settings](https://github.com/stbange/mountain/settings/pages).

Code and shaders are licensed under [MIT](LICENSE). Original sprite art, models, palette work, interface artwork and ambience are licensed under [CC BY 4.0](assets/LICENSE-ART.txt). [Art attribution](assets/CREDITS.txt) and [third-party notices](THIRD-PARTY.txt) accompany the distributions.
