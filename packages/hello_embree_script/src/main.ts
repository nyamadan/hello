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

  const files: readonly string[] = ["index.lua", "main.lua"] as const;

  async function load(file: string): Promise<void> {
    const response = await fetch(file);
    const text = await response.text();
    FS.createDataFile("/", file, text, true, true, false);
  }

  await Promise.all(files.map(load));
  callMain(["-f", files[0]]);
}

main();
