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
    try:
        page.wait_for_function(
            "document.querySelector('#loading')?.hidden && Module._mountain_value(17)===1",
            timeout=90000,
        )
    except Exception as exc:
        raise AssertionError(
            "Game startup stopped at " + page.locator("#loading").inner_text()
        ) from exc


def v(page, key):
    return page.evaluate("(key)=>Module._mountain_value(key)", key)


def frame(page):
    raw = page.evaluate(
        "()=>{Module._mountain_frame();return Module.canvas.toDataURL('image/png').split(',')[1]}"
    )
    return Image.open(io.BytesIO(base64.b64decode(raw))).convert("RGB")


def tap_key(page, key):
    page.keyboard.press(key)


def hike_beyond_lookout(page):
    # Feed time-compressed hiking input through the same compiled actions as the thumb pad.
    return page.evaluate("""() => {
      const M=Module, value=M._mountain_value;
      function walk(x,z) {
        const dx=x-value(0), dz=z-value(1);
        M._mountain_look(Math.atan2(dx,-dz)-value(2),0);
        for(let left=Math.hypot(dx,dz);left>.0001;) {
          const step=Math.min(left,.8);
          M._mountain_walk(step/5.3,1,0);
          left-=step;
        }
        if(Math.hypot(x-value(0),z-value(1))>.005 || M._mountain_wet(value(0),value(1)))
          throw Error("Could not walk the marked trail");
      }
      for(let i=1;i<value(20);i++) walk(M._mountain_path(i,0),M._mountain_path(i,1));
      let sightings=0;
      for(let i=1;i<=96;i++) {
        walk(M._mountain_extension(i,0),M._mountain_extension(i,1));
        if(i%12===0) sightings|=M._mountain_sightings();
      }
      return [value(27),value(26),sightings];
    }""")


def choose(page, name, touch=False):
    def activate(control):
        control.tap() if touch else control.click()

    activate(page.locator("#map"))
    activate(page.get_by_role("button", name="Choose a destination"))
    assert page.locator(".atlas-card").count() == 12
    page.wait_for_function(
        "Array.from(document.querySelectorAll('.atlas-card img')).every(i=>i.complete && i.naturalWidth>0)"
    )
    activate(page.get_by_role("button", name=name, exact=True))
    assert page.locator("h1").text_content() == name.split(" · ")[0]


def trail_leg(page, start, end):
    page.evaluate(
        """([start,end])=>{
        const M=Module,v=M._mountain_value;
        M._mountain_pause(1);
        for(let i=start;i<=end;i++){
            const x=M._mountain_path(i,0),z=M._mountain_path(i,1);
            let dx=x-v(0),dz=z-v(1);
            M._mountain_look(Math.atan2(dx,-dz)-v(2),0);
            for(let left=Math.hypot(dx,dz);left>.0001;){
              const step=Math.min(left,.8);M._mountain_walk(step/5.3,1,0);left-=step;
            }
            if(Math.hypot(x-v(0),z-v(1))>.01 || v(35))throw Error('Could not reach the field sign on foot');
        }
        refresh();
    }""",
        [start, end],
    )


def walk_sahara_discovery(page, url):
    choose(page, "Erg Chebbi · Sahara, Morocco")
    assert v(page, 28) == 3
    assert "Summon caravan tent" in page.locator("#cabin").inner_text()
    trail_leg(page, 1, 3)
    page.click("#use")
    assert v(page, 30) == 1
    trail_leg(page, 4, 7)
    page.click("#conditions")
    assert page.get_by_role("group", name="Caravan tent · Local phenomena").is_visible()
    assert page.get_by_role("button", name="Dust devils", exact=True).is_visible()
    page.get_by_role("button", name="Midday", exact=True).click()
    page.click("#close")
    page.click("#use")
    assert v(page, 30) == 1 and "evening" in page.locator("#toast").text_content()
    page.click("#conditions")
    page.get_by_role("button", name="Golden hour", exact=True).click()
    page.click("#close")
    page.click("#use")
    assert v(page, 30) == 2
    page.click("#guide")
    assert "Caravans' Star" in page.locator(".expedition-note").text_content()
    page.click("#close")
    trail_leg(page, 8, 14)
    page.click("#use")
    assert v(page, 30) == 3
    ready(page, url)
    choose(page, "Mare Imbrium · Earth's Moon")
    assert page.locator("#skies").text_content() == "Airless"
    page.click("#guide")
    assert "Caravans' Star" in page.locator(".expedition-note").text_content()
    page.click("#close")
    choose(page, "Erg Chebbi · Sahara, Morocco")
    assert v(page, 30) == 3
    page.click("#cabin")
    page.click("#use")
    page.click("#use")
    assert v(page, 11) == 1
    page.get_by_role("button", name="Hearth room", exact=False).click()
    page.click("#use")
    page.click("#cabin")


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
    before = page.locator(".map-frame").evaluate("(c)=>c.toDataURL()")
    page.get_by_role("button", name="Summit route", exact=True).click()
    assert page.locator(".map-frame").evaluate("(c)=>c.toDataURL()") != before
    page.get_by_role("button", name="Local map", exact=True).click()
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
    # Walking up to a closed door discovers nothing. Opening it reveals the room before crossing.
    page.mouse.move(500, 350)
    page.mouse.down()
    page.mouse.move(540, 350)
    page.mouse.up()
    page.keyboard.down("w")
    page.wait_for_function("Module._mountain_value(13)===8", timeout=30000)
    page.keyboard.up("w")
    assert v(page, 12) == 0 and v(page, 38) == 1
    page.click("#use")
    assert v(page, 12) == 0 and v(page, 38) == 2
    page.click("#use")
    assert v(page, 12) == 1
    tap_key(page, "r")
    assert v(page, 12) == 0
    page.click("#use")
    assert v(page, 11) == 0
    page.click("#cabin")
    assert v(page, 9) == 0
    page.click("#conditions")
    page.get_by_role("button", name="Winter", exact=True).click()
    page.get_by_role("button", name="Snow", exact=True).click()
    assert v(page, 18) == 0 and v(page, 19) == 0
    page.get_by_label("Cycle through seasons").check()
    assert v(page, 18) == 1 and v(page, 19) == 0
    page.get_by_label("Let the weather change").check()
    page.get_by_role("button", name="Winter", exact=True).click()
    assert v(page, 18) == 0 and v(page, 19) == 1
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
    page.get_by_role("button", name="Taggart Lake · Wyoming").click()
    assert v(page, 7) == v(page, 23) and v(page, 24) == 5
    assert page.locator("h1").text_content() == "Taggart Lake"
    assert 2100 < v(page, 8) < 2110 and v(page, 22) > 150
    page.click("#map")
    assert page.get_by_text("① Lakeside View", exact=False).is_visible()
    page.click("#close")
    distance, station, sightings = hike_beyond_lookout(page)
    assert distance > 2000 and abs(station - 96) < 0.001 and sightings > 0
    assert max(ImageStat.Stat(frame(page)).stddev) > 10
    page.click("#map")
    assert page.get_by_text("Endless trail", exact=False).is_visible()
    assert page.get_by_text("m walked", exact=False).is_visible()
    page.get_by_role("button", name="Generate a new mountain").click()
    assert v(page, 7) != 4812
    page.click("#guide")
    assert page.get_by_text("Use a door or read a field sign", exact=True).is_visible()
    assert page.get_by_text("Wildlife nearby", exact=True).is_visible()
    assert page.get_by_text("Mule deer", exact=False).is_visible()
    page.keyboard.press("Escape")
    walk_sahara_discovery(page, url)
    assert not errors, errors
    ctx.close()
    print(
        "Desktop hiking, summit map, all twelve atlas photographs, Sahara discovery, journal reload, cabin, climate and PNG passed.",
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
    page.get_by_role("button", name="Season and weather", exact=True).tap()
    page.get_by_role("button", name="Spring", exact=True).tap()
    page.get_by_role("button", name="Mist", exact=True).tap()
    page.tap("#close")
    assert (v(page, 5), v(page, 6)) == (0, 3)
    choose(page, "Sarakiniko · Milos, Greece", touch=True)
    assert v(page, 28) == 2
    assert "Summon cycladic boathouse" in page.locator("#cabin").inner_text()
    page.tap("#cabin")
    page.tap("#use")
    page.tap("#door")
    assert not v(page, 10) and not v(page, 11)
    page.tap("#use")
    page.tap("#use")
    assert v(page, 11)
    bearing = v(page, 2) + 0.16
    room_walk(page, bearing, -3.9, 0.85)
    assert v(page, 13) == 9
    page.tap("#use")
    page.tap("#blind")
    assert v(page, 13) == 10 and v(page, 42)
    page.tap("#cabin")
    assert v(page, 13) == 3
    page.set_viewport_size({"width": 800, "height": 390})
    assert page.locator("#camera").is_visible()
    assert page.evaluate("document.documentElement.scrollWidth===window.innerWidth")
    ctx.close()
    print(
        "Real browser touch movement, looking, door controls and rotation passed.",
        flush=True,
    )


def room_walk(page, bearing, dx, dz):
    # Relative metre offsets driven through the same walk and look inputs as the keyboard.
    page.evaluate(
        """([bearing,dx,dz])=>{
      const m=Module, dir=bearing+Math.atan2(dx,-dz);
      m._mountain_look(dir-val(2),-val(3));
      let left=Math.hypot(dx,dz)/3.71;
      while(left>0){const dt=Math.min(.08,left);m._mountain_walk(dt,1,0);left-=dt;}
      refresh();
    }""",
        [bearing, dx, dz],
    )


def test_cabin(browser, url):
    ctx = browser.new_context(viewport={"width": 1100, "height": 720})
    page = ctx.new_page()
    ready(page, url)
    page.evaluate("Module._mountain_pause(1)")
    bearing = v(page, 2)
    page.keyboard.press("r")
    assert v(page, 11) and v(page, 38) == 1
    room_walk(page, bearing, 3.6, -2.65)
    assert v(page, 13) == 8
    before = frame(page)
    page.click("#use")
    preview = frame(page)
    assert (
        v(page, 38) == 2 and v(page, 12) == 0 and before.tobytes() != preview.tobytes()
    )
    page.click("#use")
    assert page.locator("h1").text_content() == "The winter garden"
    turn = bearing + 1.57079632679
    room_walk(page, turn, -11.45, -8.15)
    assert v(page, 13) == 11
    page.click("#use")
    page.click("#use")
    assert v(page, 11) and v(page, 13) == 12
    page.click("#conditions")
    page.get_by_role("button", name="Heavy fog", exact=True).click()
    page.get_by_label("Watch a wolf pack").check()
    page.get_by_label("Watch a ridge giant").check()
    page.keyboard.press("Escape")
    assert v(page, 39) == 3 and v(page, 40) and v(page, 41) and v(page, 35) == 0
    page.evaluate("Module._mountain_pause(1)")
    page.click("#use")
    room_walk(page, turn, -0.32, -5.4)
    assert v(page, 13) == 9
    # Face the window from the gallery so its independent mechanisms occupy the frame.
    page.evaluate(
        "yaw=>{Module._mountain_look(yaw-val(2),-val(3));refresh()}",
        turn - 1.57079632679,
    )
    glazed = frame(page)
    page.evaluate("Module._mountain_action(11)")
    clear_view = frame(page)
    assert v(page, 39) == 0 and clear_view.tobytes() != glazed.tobytes()
    page.evaluate("()=>{for(let i=0;i<3;i++)Module._mountain_action(11)}")
    page.click("#use")
    plain = frame(page)
    assert v(page, 13) == 10 and glazed.tobytes() != plain.tobytes()
    page.click("#blind")
    shutter = frame(page)
    assert v(page, 42) and shutter.tobytes() != plain.tobytes()
    page.keyboard.press("r")
    assert not v(page, 12)
    room_walk(page, bearing, 3.6, -2.65)
    page.click("#door")
    assert v(page, 13) == 8
    ready(page, url)
    assert v(page, 38) == 2 and v(page, 39) == 3 and v(page, 40) and v(page, 41)
    page.evaluate("()=>{Module._mountain_pause(1);Module._mountain_visit(7);refresh()}")
    bearing = v(page, 2)
    page.keyboard.press("r")
    room_walk(page, bearing, 3.6, -2.65)
    assert v(page, 13) == 8
    page.click("#use")
    page.click("#use")
    turn = bearing + 1.57079632679
    room_walk(page, turn, -11.45, -13.55)
    assert v(page, 13) == 10 and v(page, 42) and v(page, 38) == 2
    page.click("#blind")
    assert not v(page, 42)
    page.click("#conditions")
    page.get_by_role("button", name="Calm", exact=True).click()
    assert page.get_by_role(
        "button", name="Micrometeor impacts", exact=True
    ).is_visible()
    assert page.get_by_role("button", name="Heavy fog", exact=True).count() == 0
    page.get_by_label("Watch a runaway survey rover").uncheck()
    page.get_by_label("Watch a crater-wall landslide").uncheck()
    assert not v(page, 39) and not v(page, 40) and not v(page, 41)
    ctx.close()
    print(
        "Cabin doors, room previews, balcony, panes, blinds and cross-world save reload passed.",
        flush=True,
    )


def test_lighting(browser, url):
    ctx = browser.new_context(viewport={"width": 1170, "height": 740})
    page = ctx.new_page()
    ready(page, url)
    page.evaluate("()=>{Module._mountain_pause(1);Module._mountain_visit(0)}")
    trail_leg(page, 1, int(v(page, 20)) - 1)
    # Face away from the door. It remains outside the image; only its cast shadow can reach the rug.
    page.evaluate("""()=>{
      const m=Module;
      m._mountain_conditions(1,0,15.5);m._mountain_look(2.24-val(2),0);
      m._mountain_action(1);m._mountain_action(2);m._mountain_action(2);m._mountain_action(6);
      m._mountain_look(2.24-val(2),-.88-val(3));refresh();
    }""")

    def patch(image, left, top, right, bottom):
        w, h = image.size
        region = image.crop(
            (int(left * w), int(top * h), int(right * w), int(bottom * h))
        )
        return ImageStat.Stat(region).mean[0]

    sun = frame(page)
    page.keyboard.press("f")
    assert v(page, 11) and not v(page, 10)
    shade = frame(page)
    rug = (0.44, 0.59, 0.57, 0.83)
    # The adjacent strip lit by the window stays bright when the door closes.
    window = (0.075, 0.49, 0.13, 0.61)
    assert patch(sun, *rug) > patch(shade, *rug) + 14
    assert abs(patch(sun, *window) - patch(shade, *window)) < 7
    page.click("#use")
    reopened = frame(page)
    assert abs(patch(sun, *rug) - patch(reopened, *rug)) < 2
    page.evaluate("Module._mountain_conditions(1,0,23)")
    night = frame(page)
    assert patch(night, *rug) < patch(sun, *rug)
    # With the same seed, hour, pose and simulation tick, lunar weather has no optical effect.
    page.evaluate("()=>{Module._mountain_visit(7);Module._mountain_conditions(1,0,9)}")
    clear = frame(page)
    page.evaluate("Module._mountain_conditions(1,2,9)")
    assert frame(page).tobytes() == clear.tobytes()
    page.evaluate(
        "()=>{for(const action of [11,12,13])Module._mountain_action(action)}"
    )
    assert frame(page).tobytes() == clear.tobytes()
    assert page.evaluate("Module.canvas.getContext('webgl2').getError()") == 0
    ctx.close()
    print(
        "Sunlit rug, persistent window light, nightfall and airless lunar lighting passed.",
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
    stereo = page.evaluate("testHeadset.shadowViews")
    assert len(stereo) == 2 and stereo[0] == stereo[1] and any(stereo[0])
    assert eyes[0][1] > 20 and eyes[1][1] > 20 and eyes[0][0] != eyes[1][0], eyes
    assert v(page, 15) > 0
    page.evaluate("testHeadset.session.inputSources[0].gamepad.axes=[0,0,0,0]")
    page.evaluate(
        "testHeadset.yaw=.4;testHeadset.session.dispatchEvent(new Event('squeeze'))"
    )
    assert v(page, 9)
    page.evaluate("testHeadset.session.dispatchEvent(new Event('select'))")
    assert v(page, 10)
    page.evaluate("""()=>{const e=new Event('squeeze'); e.inputSource={handedness:'left'};
        testHeadset.session.dispatchEvent(e)}""")
    assert not v(page, 10)
    page.evaluate(
        "testHeadset.session.inputSources[0].gamepad.buttons[4]={pressed:true}"
    )
    page.wait_for_function("Module._mountain_value(5)===2")
    page.evaluate(
        "testHeadset.session.inputSources[0].gamepad.buttons[5]={pressed:true}"
    )
    page.wait_for_function("Module._mountain_value(6)===1")
    assert v(page, 5) == 2
    page.evaluate("""()=>{
      const session=testHeadset.session;
      session.inputSources[0].gamepad.axes=[0,0,0,0];
      session.dispatchEvent(new Event('select'));
      session.dispatchEvent(new Event('select'));
    }""")
    assert v(page, 11)
    room_walk(page, v(page, 2) + 0.16, -3.9, 0.85)
    assert v(page, 13) == 9
    page.evaluate("""()=>{
      const session=testHeadset.session;
      session.dispatchEvent(new Event('select'));
      const e=new Event('squeeze');e.inputSource={handedness:'left'};session.dispatchEvent(e);
    }""")
    assert v(page, 13) == 10 and v(page, 42)
    page.evaluate("testHeadset.session.dispatchEvent(new Event('squeeze'))")
    assert not v(page, 12) and v(page, 13) == 3
    page.evaluate("testHeadset.session.end()")
    assert page.evaluate("mountainXR.session===null")
    assert max(ImageStat.Stat(frame(page)).stddev) > 15
    choose(page, "Mare Imbrium · Earth's Moon")
    trail_leg(page, 1, 3)
    page.click("#guide")
    page.get_by_role("button", name="Enter VR").click()
    page.evaluate("testHeadset.session.inputSources[0].gamepad.axes=[0,0,0,0]")
    page.evaluate("testHeadset.session.dispatchEvent(new Event('select'))")
    assert v(page, 30) == 1
    trail_leg(page, 4, 7)
    page.evaluate("""()=>{let target=Math.atan2(Module._mountain_path(14,0)-val(0),val(1)-Module._mountain_path(14,1));
        Module._mountain_look(target+1-val(2),0);testHeadset.yaw=0;window.eyeCount=testHeadset.frames;}""")
    page.wait_for_function("testHeadset.frames>window.eyeCount+1")
    page.evaluate("testHeadset.session.dispatchEvent(new Event('select'))")
    assert v(page, 30) == 1
    page.evaluate("testHeadset.yaw=1;window.eyeCount=testHeadset.frames")
    page.wait_for_function("testHeadset.frames>window.eyeCount+1")
    page.evaluate("testHeadset.session.dispatchEvent(new Event('select'))")
    assert v(page, 30) == 2
    page.evaluate("testHeadset.session.end()")
    ctx.close()
    print(
        "Emulated WebXR stereo, locomotion, cabin input, lunar sight alignment with head tracking and session exit passed.",
        flush=True,
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("build", nargs="?", default="build-web/play")
    p.add_argument("--url")
    args = p.parse_args()
    with site(args) as url, sync_playwright() as driver:
        for playtest in (
            test_desktop,
            test_touch,
            test_headset,
            test_lighting,
            test_cabin,
        ):
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
                playtest(browser, url)
            finally:
                browser.close()
