"""Exercise the staged or deployed game using real keyboard, touch and WebGL draws."""

import argparse
import base64
import contextlib
import functools
import io
import os
from pathlib import Path
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread
from tempfile import TemporaryDirectory
from PIL import Image, ImageStat
from playwright.sync_api import sync_playwright


@contextlib.contextmanager
def site(args):
    if args.url:
        yield args.url
        return

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def translate_path(self, path):
            # Host at the same project subpath used by GitHub Pages.
            if not path.startswith("/mountain/"):
                return "/not-a-game-route"
            return super().translate_path(path[len("/mountain") :])

    server = ThreadingHTTPServer(
        ("127.0.0.1", 0),
        functools.partial(Handler, directory=str(Path(args.build).resolve())),
    )
    thread = Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield f"http://127.0.0.1:{server.server_port}/mountain/"
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


def ready(page, url):
    page.goto(url, wait_until="domcontentloaded")
    page.wait_for_function(
        "document.querySelector('#loading')?.hidden && Module._mountain_value(17)===1",
        timeout=90000,
    )


def v(page, key):
    return page.evaluate("(key)=>Module._mountain_value(key)", key)


def frame(page):
    raw = page.evaluate(
        "()=>{Module._mountain_frame();return Module.canvas.toDataURL('image/png').split(',')[1]}"
    )
    return Image.open(io.BytesIO(base64.b64decode(raw))).convert("RGB")


def tap_key(page, key):
    page.keyboard.press(key)


def test_desktop(browser, url):
    ctx = browser.new_context(
        viewport={"width": 1200, "height": 760}, accept_downloads=True
    )
    page = ctx.new_page()
    errors = []
    page.on("pageerror", lambda error: errors.append(str(error)))
    ready(page, url)
    original = frame(page)
    assert max(ImageStat.Stat(original).stddev) > 15
    assert page.locator("h1").text_content() == "Larkspur Ridge"
    page.keyboard.down("w")
    page.wait_for_function("Module._mountain_value(15)>.08")
    page.keyboard.up("w")
    angle = v(page, 2)
    page.mouse.move(580, 310)
    page.mouse.down()
    page.mouse.move(615, 296, steps=4)
    page.mouse.up()
    assert abs(v(page, 2) - angle) > 0.04
    page.click("#map")
    assert page.locator(".map-frame").evaluate("(c)=>c.width") > 100
    page.fill("#seed", "4812")
    page.get_by_role("button", name="Hike", exact=True).click()
    assert v(page, 15) == 0
    page.click("#cabin")
    assert v(page, 9) == 1 and v(page, 10) == 0
    assert frame(page).tobytes() != original.tobytes()
    page.click("#use")
    assert v(page, 10) == 1 and v(page, 11) == 0
    page.click("#use")
    assert v(page, 11) == 1
    # Walk through the far door with keyboard input.
    page.mouse.move(500, 350)
    page.mouse.down()
    page.mouse.move(540, 350)
    page.mouse.up()
    page.keyboard.down("w")
    page.wait_for_function("Module._mountain_value(12)>=1", timeout=30000)
    page.keyboard.up("w")
    tap_key(page, "r")
    assert v(page, 12) == 0
    page.click("#use")
    assert v(page, 11) == 0
    page.click("#cabin")
    assert v(page, 9) == 0
    page.click("#conditions")
    page.get_by_role("button", name="Winter", exact=True).click()
    page.get_by_role("button", name="Rain", exact=True).click()
    page.get_by_role("button", name="Midnight", exact=True).click()
    page.click("#close")
    night = frame(page)
    assert v(page, 5) == 3 and v(page, 6) == 2
    assert (
        ImageStat.Stat(night.convert("L")).mean[0]
        < ImageStat.Stat(original.convert("L")).mean[0] * 0.74
    )
    page.click("#camera")
    page.click("#grid")
    assert page.locator("#thirds").is_visible()
    page.click("#shutter")
    page.wait_for_function("document.querySelector('#count').textContent==='1'")
    page.wait_for_function(
        "document.querySelector('#toast').textContent.includes('saved to your journal')"
    )
    page.click("#journal")
    page.locator(".print").first.click()
    with page.expect_download() as download:
        page.get_by_role("button", name="Download PNG").click()
    with TemporaryDirectory() as temp:
        target = Path(temp) / "shot.png"
        download.value.save_as(target)
        photo = Image.open(target)
        assert photo.format == "PNG" and photo.size == night.size
        assert max(ImageStat.Stat(photo.convert("RGB")).stddev) > 3
    page.reload()
    page.wait_for_function(
        "document.querySelector('#loading').hidden && document.querySelector('#count').textContent==='1'"
    )
    page.click("#journal")
    assert page.locator(".print").count() == 1
    page.click("#close")
    page.click("#map")
    page.get_by_role("button", name="Generate a new mountain").click()
    assert v(page, 7) != 4812
    page.click("#guide")
    assert page.get_by_text("Open a door or cross it", exact=True).is_visible()
    page.keyboard.press("Escape")
    assert not errors, errors
    ctx.close()
    print(
        "Desktop hike, cabin, climate, map, camera, PNG download and persistent journal passed.",
        flush=True,
    )


def test_touch(browser, url):
    ctx = browser.new_context(
        viewport={"width": 390, "height": 844},
        is_mobile=True,
        has_touch=True,
        device_scale_factor=1,
    )
    page = ctx.new_page()
    ready(page, url)
    pad = page.locator("#pad")
    assert pad.is_visible()
    client = ctx.new_cdp_session(page)

    def event(kind, x=0, y=0):
        client.send(
            "Input.dispatchTouchEvent",
            {
                "type": kind,
                "touchPoints": []
                if kind == "touchEnd"
                else [{"x": x, "y": y, "id": 1}],
            },
        )

    b = pad.bounding_box()
    x = b["x"] + b["width"] / 2
    y = b["y"] + b["height"] / 2
    event("touchStart", x, y)
    event("touchMove", x, y - 26)
    page.wait_for_function("Module._mountain_value(15)>.05")
    event("touchEnd")
    angle = v(page, 2)
    event("touchStart", 150, 290)
    event("touchMove", 204, 300)
    event("touchEnd")
    assert abs(v(page, 2) - angle) > 0.1
    page.tap("#cabin")
    page.tap("#use")
    page.tap("#use")
    assert v(page, 11)
    page.set_viewport_size({"width": 800, "height": 390})
    assert page.locator("#camera").is_visible()
    assert page.evaluate("document.documentElement.scrollWidth===window.innerWidth")
    ctx.close()
    print(
        "Real browser touch movement, looking, door controls and rotation passed.",
        flush=True,
    )


def test_headset(browser, url):
    ctx = browser.new_context(viewport={"width": 880, "height": 540})
    ctx.add_init_script(path=str(Path(__file__).with_name("headset.js")))
    page = ctx.new_page()
    ready(page, url)
    page.click("#guide")
    page.get_by_role("button", name="Enter VR").click()
    page.wait_for_function("testHeadset.frames>=3")
    eyes = page.evaluate("testHeadset.lastEyes")
    assert eyes[0][1] > 20 and eyes[1][1] > 20 and eyes[0][0] != eyes[1][0]
    assert v(page, 15) > 0
    page.evaluate(
        "testHeadset.yaw=.4;testHeadset.session.dispatchEvent(new Event('squeeze'))"
    )
    assert v(page, 9)
    page.evaluate("testHeadset.session.dispatchEvent(new Event('select'))")
    assert v(page, 10)
    page.evaluate("testHeadset.session.end()")
    assert page.evaluate("mountainXR.session===null")
    assert max(ImageStat.Stat(frame(page)).stddev) > 15
    ctx.close()
    print(
        "Emulated WebXR stereo frames, tracking, locomotion, cabin input and session exit passed.",
        flush=True,
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("build", nargs="?", default="build-web/play")
    p.add_argument("--url")
    args = p.parse_args()
    with site(args) as url, sync_playwright() as driver:
        browser = driver.chromium.launch(
            executable_path=os.environ.get("CHROMIUM") or None,
            args=[
                "--no-sandbox",
                "--use-angle=swiftshader",
                "--enable-unsafe-swiftshader",
                "--disable-dev-shm-usage",
            ],
        )
        try:
            test_desktop(browser, url)
            test_touch(browser, url)
            test_headset(browser, url)
        finally:
            browser.close()
