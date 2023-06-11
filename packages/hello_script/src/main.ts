// @ts-ignore
import createModule from "hello_host";

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

  type AssetFileName = typeof assetFileNames[number];
  const assetFileNames = [
    "index.lua",
    "main.lua",
    "inspect.lua",
    "handle_error.lua",
    "uv_checker.png",
  ] as const;
  const [entry] = assetFileNames;

  async function load<T extends AssetFileName = AssetFileName>(file: T) {
    const response = await fetch(file);
    const arrayBuffer = await response.arrayBuffer();
    const buffer = new Uint8Array(arrayBuffer);
    FS.createDataFile("/", file, buffer, true, true, false);

    return [file, buffer];
  }

  await Promise.all(assetFileNames.map(load));
  callMain([entry]);
}

main();
