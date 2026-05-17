"use client";

import { useEffect, useRef } from "react";

export default function Aurora() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const gl = canvas.getContext("webgl", { alpha: true, premultipliedAlpha: false });
    if (!gl) return;

    function resize() {
      const dpr = Math.min(window.devicePixelRatio, 2);
      canvas!.width = canvas!.offsetWidth * dpr;
      canvas!.height = canvas!.offsetHeight * dpr;
      gl!.viewport(0, 0, canvas!.width, canvas!.height);
    }
    window.addEventListener("resize", resize);
    resize();

    const vs = "attribute vec2 p; void main(){gl_Position=vec4(p,0,1);}";
    const fs = `
      precision mediump float;
      uniform float t;
      uniform vec2  r;
      vec3 hue(float h) {
        float h6 = fract(h) * 6.0;
        return clamp(vec3(
          abs(h6 - 3.0) - 1.0,
          2.0 - abs(h6 - 2.0),
          2.0 - abs(h6 - 4.0)
        ), 0.0, 1.0);
      }
      void main() {
        vec2 uv = gl_FragCoord.xy / r;
        uv.y = 1.0 - uv.y;
        float wave = sin(uv.x * 10.0 + t * 0.9) * 0.03
                   + sin(uv.x * 4.0  - t * 0.6) * 0.05
                   + sin(uv.x * 22.0 + t * 1.4) * 0.01;
        float band = uv.y + wave;
        float h = fract(uv.x * 0.8 - t * 0.07);
        vec3 col = hue(h);
        float glow = exp(-band * 10.0) * 0.55;
        glow *= 1.0 - pow(abs(uv.x - 0.5) * 1.8, 3.0);
        gl_FragColor = vec4(col * glow, glow * 0.9);
      }
    `;

    function mkShader(type: number, src: string) {
      const s = gl!.createShader(type);
      if (!s) return null;
      gl!.shaderSource(s, src);
      gl!.compileShader(s);
      return s;
    }

    const prog = gl.createProgram();
    if (!prog) return;
    const vsh = mkShader(gl.VERTEX_SHADER, vs);
    const fsh = mkShader(gl.FRAGMENT_SHADER, fs);
    if (!vsh || !fsh) return;
    gl.attachShader(prog, vsh);
    gl.attachShader(prog, fsh);
    gl.linkProgram(prog);
    gl.useProgram(prog);

    const buf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
    const loc = gl.getAttribLocation(prog, "p");
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);

    const uT = gl.getUniformLocation(prog, "t");
    const uR = gl.getUniformLocation(prog, "r");

    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    let start: number | null = null;
    function frame(ts: number) {
      if (!start) start = ts;
      gl!.clearColor(0, 0, 0, 0);
      gl!.clear(gl!.COLOR_BUFFER_BIT);
      gl!.uniform1f(uT!, (ts - start) / 1000);
      gl!.uniform2f(uR!, canvas!.width, canvas!.height);
      gl!.drawArrays(gl!.TRIANGLE_STRIP, 0, 4);
      animId = requestAnimationFrame(frame);
    }
    let animId = requestAnimationFrame(frame);

    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener("resize", resize);
    };
  }, []);

  return <canvas ref={canvasRef} id="aurora" />;
}
