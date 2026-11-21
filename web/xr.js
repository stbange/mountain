"use strict";
window.mountainXR = {
  supported: false,
  session: null,
  async probe() {
    try {
      this.supported = Boolean(
        navigator.xr && (await navigator.xr.isSessionSupported("immersive-vr")),
      );
    } catch {
      this.supported = false;
    }
  },
  async start() {
    if (this.session) return;
    let session;
    try {
      session = await navigator.xr.requestSession("immersive-vr", {
        optionalFeatures: ["local-floor"],
      });
      this.session = session;
      const gl = canvas.getContext("webgl2");
      await gl.makeXRCompatible();
      const layer = new XRWebGLLayer(session, gl, {
        alpha: false,
        depth: true,
        antialias: false,
      });
      session.updateRenderState({
        baseLayer: layer,
        depthNear: 0.08,
        depthFar: 2400,
      });
      const ref = await session.requestReferenceSpace("local");
      let last = 0,
        turned = false,
        summoned = false,
        timeHeld = false,
        seasonHeld = false,
        weatherHeld = false,
        headYaw = 0;
      const leave = () => {
        this.session = null;
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        Module._mountain_xr_active(0);
        stop();
        resize();
      };
      session.addEventListener("end", leave, { once: true });
      session.addEventListener("select", () => {
        Module._mountain_observe(headYaw);
        refresh();
      });
      session.addEventListener("squeeze", () => act(1));
      Module._mountain_xr_active(1);
      const frame = (now, xrFrame) => {
        if (this.session !== session) return;
        const pose = xrFrame.getViewerPose(ref);
        if (pose) {
          const elapsed = last ? Math.min(0.1, (now - last) / 1000) : 0;
          let forward = 0,
            right = 0,
            turn = 0,
            button = false,
            season = false,
            weather = false,
            time = false;
          for (const source of session.inputSources) {
            const gp = source.gamepad;
            if (!gp) continue;
            const axes = gp.axes,
              x = axes[axes.length >= 4 ? 2 : 0] || 0,
              y = axes[axes.length >= 4 ? 3 : 1] || 0;
            if (source.handedness === "right") {
              turn = x;
              button = Boolean(gp.buttons[4]?.pressed);
              time = Boolean(gp.buttons[5]?.pressed);
            } else {
              season = Boolean(gp.buttons[4]?.pressed);
              weather = Boolean(gp.buttons[5]?.pressed);
              right = Math.abs(x) > 0.15 ? x : 0;
              forward = Math.abs(y) > 0.15 ? -y : 0;
            }
          }
          if (Math.abs(turn) > 0.7 && !turned)
            Module._mountain_look((Math.sign(turn) * Math.PI) / 6, 0);
          turned = Math.abs(turn) > 0.5;
          if (season && !seasonHeld) act(4);
          if (weather && !weatherHeld) act(5);
          if (time && !timeHeld) act(3);
          timeHeld = time;
          seasonHeld = season;
          weatherHeld = weather;
          if (button && !summoned) act(1);
          summoned = button;
          const rot = pose.transform.orientation;
          headYaw = Math.atan2(
            -2 * (rot.w * rot.y + rot.x * rot.z),
            1 - 2 * (rot.y * rot.y + rot.x * rot.x),
          );
          Module._mountain_walk(
            elapsed,
            forward * Math.cos(headYaw) - right * Math.sin(headYaw),
            right * Math.cos(headYaw) + forward * Math.sin(headYaw),
          );
          gl.bindFramebuffer(gl.FRAMEBUFFER, layer.framebuffer);
          for (const view of pose.views) {
            const vp = layer.getViewport(view);
            Module.ccall(
              "mountain_xr_eye",
              null,
              ["array", "array", "number", "number", "number", "number"],
              [
                new Uint8Array(
                  view.projectionMatrix.buffer,
                  view.projectionMatrix.byteOffset,
                  64,
                ),
                new Uint8Array(
                  view.transform.inverse.matrix.buffer,
                  view.transform.inverse.matrix.byteOffset,
                  64,
                ),
                vp.x,
                vp.y,
                vp.width,
                vp.height,
              ],
            );
          }
          gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        }
        last = now;
        session.requestAnimationFrame(frame);
      };
      session.requestAnimationFrame(frame);
    } catch (error) {
      this.session = null;
      Module._mountain_xr_active(0);
      if (session) await session.end().catch(() => {});
      notice("VR could not start. " + error.message);
    }
  },
};
