Module["postRun"] = async function() {
  const response = await fetch("/main.lua");
  const text = await response.text();
  FS.createDataFile("/", "main.lua", text, true, true, false);
  callMain(["-f", "./main.lua"]);
}
