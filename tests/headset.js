// Browser fixture for a tracked headset. The framebuffer, shaders and eye draws
// still use the browser's WebGL2 implementation and the compiled C renderer.
(() => {
  const I = () =>
    new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
  const rig = (window.testHeadset = {
    frames: 0,
    yaw: 0,
    lastEyes: [],
    rightStick: 0,
  });
  class Session extends EventTarget {
    constructor() {
      super();
      this.active = true;
      this.inputSources = [
        { handedness: "left", gamepad: { axes: [0, 0, 0, -0.8], buttons: [] } },
        { handedness: "right", gamepad: { axes: [0, 0, 0, 0], buttons: [] } },
      ];
    }
    updateRenderState(state) {
      this.layer = state.baseLayer;
    }
    async requestReferenceSpace(type) {
      if (type !== "local") throw Error(type);
      return {};
    }
    requestAnimationFrame(cb) {
      return requestAnimationFrame((now) => {
        if (!this.active) return;
        const yaw = rig.yaw,
          proj = I();
        proj[0] = 1.4;
        proj[5] = 1.4;
        proj[10] = -1.0001;
        proj[11] = -1;
        proj[14] = -0.16;
        proj[15] = 0;
        const eyes = ["left", "right"].map((eye, i) => {
          const m = I();
          m[0] = Math.cos(yaw);
          m[2] = -Math.sin(yaw);
          m[8] = Math.sin(yaw);
          m[10] = Math.cos(yaw);
          m[12] = i ? -0.032 : 0.032;
          return {
            eye,
            projectionMatrix: proj,
            transform: { inverse: { matrix: m } },
          };
        });
        const pose = {
          views: eyes,
          transform: {
            orientation: {
              x: 0,
              y: Math.sin(yaw / 2),
              z: 0,
              w: Math.cos(yaw / 2),
            },
          },
        };
        const gl = this.layer.gl;
        const call = Module.ccall;
        rig.shadowViews = [];
        Module.ccall = function (name, ...args) {
          const result = call(name, ...args);
          if (name === "mountain_xr_eye") {
            const program = gl.getParameter(gl.CURRENT_PROGRAM);
            const matrices = ["u_near_light", "u_far_light"].flatMap(
              (uniform) =>
                Array.from(
                  gl.getUniform(
                    program,
                    gl.getUniformLocation(program, uniform),
                  ),
                ),
            );
            rig.shadowViews.push(matrices);
          }
          return result;
        };
        try {
          cb(now, { getViewerPose: () => pose });
        } finally {
          Module.ccall = call;
        }
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.layer.framebuffer);
        const pixels = new Uint8Array(160 * 80 * 4);
        gl.readPixels(0, 0, 160, 80, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
        rig.lastEyes = [0, 1].map((i) => {
          let h = 2166136261,
            lo = 255,
            hi = 0;
          for (let y = 0; y < 80; y += 2)
            for (let x = 0; x < 80; x += 2) {
              const p = (y * 160 + i * 80 + x) * 4;
              lo = Math.min(lo, pixels[p]);
              hi = Math.max(hi, pixels[p]);
              h = Math.imul(h ^ pixels[p], 16777619);
            }
          return [h, hi - lo];
        });
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        rig.frames++;
      });
    }
    async end() {
      this.active = false;
      this.dispatchEvent(new Event("end"));
      this.layer?.close();
    }
  }
  class Layer {
    constructor(session, gl) {
      this.gl = gl;
      this.framebuffer = gl.createFramebuffer();
      gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffer);
      this.colour = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, this.colour);
      gl.texImage2D(
        gl.TEXTURE_2D,
        0,
        gl.RGBA,
        160,
        80,
        0,
        gl.RGBA,
        gl.UNSIGNED_BYTE,
        null,
      );
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
      gl.framebufferTexture2D(
        gl.FRAMEBUFFER,
        gl.COLOR_ATTACHMENT0,
        gl.TEXTURE_2D,
        this.colour,
        0,
      );
      this.depth = gl.createRenderbuffer();
      gl.bindRenderbuffer(gl.RENDERBUFFER, this.depth);
      gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, 160, 80);
      gl.framebufferRenderbuffer(
        gl.FRAMEBUFFER,
        gl.DEPTH_ATTACHMENT,
        gl.RENDERBUFFER,
        this.depth,
      );
      if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) !== gl.FRAMEBUFFER_COMPLETE)
        throw Error("XR test framebuffer incomplete");
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    }
    getViewport(view) {
      return { x: view.eye === "left" ? 0 : 80, y: 0, width: 80, height: 80 };
    }
    close() {
      const g = this.gl;
      g.deleteTexture(this.colour);
      g.deleteRenderbuffer(this.depth);
      g.deleteFramebuffer(this.framebuffer);
    }
  }
  Object.defineProperty(navigator, "xr", {
    value: {
      isSessionSupported: async (mode) => mode === "immersive-vr",
      requestSession: async (mode) => {
        if (mode !== "immersive-vr") throw Error("Unexpected mode");
        return (rig.session = new Session());
      },
    },
  });
  Object.defineProperty(window, "XRWebGLLayer", {
    value: Layer,
    configurable: true,
  });
  WebGL2RenderingContext.prototype.makeXRCompatible = async () => {};
})();
