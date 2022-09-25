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

  const response = await fetch("/main.lua");
  const text = await response.text();
  FS.createDataFile("/", "main.lua", text, true, true, false);
  callMain(["-f", "/main.lua"]);
}

main();
