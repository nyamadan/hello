// @ts-ignore
import createModule from "hello_embree_host";

async function main() {
  const Module: any = await createModule();
  const { FS, callMain } = Module;

  const canvas = document.querySelector("#canvas");
  if (canvas == null) {
    throw new Error("Could not find #canvas");
  }
  canvas.addEventListener(
    "webglcontextlost",
    function (e) {
      alert("WebGL context lost. You will need to reload the page.");
      e.preventDefault();
    },
    false
  );
  Module["canvas"] = canvas;

  const texts: readonly string[] = [
    "index.lua",
    "inspect.lua",
    "handle_error.lua",
    "main.lua",
  ] as const;

  const assets: readonly string[] = ["uv_checker.png"] as const;

  async function loadText(file: string): Promise<void> {
    const response = await fetch(file);
    const text = await response.text();
    FS.createDataFile("/", file, text, true, true, false);
  }

  async function loadBinary(file: string): Promise<void> {
    const response = await fetch(file);
    const arrayBuffer = await response.arrayBuffer();
    const buffer = new Uint8Array(arrayBuffer);
    FS.createDataFile("/", file, buffer, true, true, false);
  }

  await Promise.all([...texts.map(loadText), ...assets.map(loadBinary)]);
  callMain(["-f", texts[0]]);
}

main();
