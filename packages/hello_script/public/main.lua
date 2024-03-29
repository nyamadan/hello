local utils = require("utils")
--- @type gl
local GL = require("opengl")
--- @type SDL
local SDL = require("sdl2")
--- @type SDL_image
local SDL_image = require("sdl2_image")
local GLSLANG = require("glslang")
local SPV_CROSS = require("spv_cross")

local handleError = require("handle_error")
local inspect = require("inspect")

---Transpile Shaders
---@return string VertexShader
---@return string FragmentShader
local function transpileShaders()
    local DefaultVertexShader = [[#version 310 es
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aUv0;

out vec2 vUv0;

void main(void) {
    vUv0 = aUv0;
    gl_Position = vec4(aPosition, 1.0);
}
]]

    local DefaultFragmentShader = [[#version 310 es
precision mediump float;

out vec4 fragColor;

in vec2 vUv0;

layout(location=0) uniform sampler2D uSampler;

void main(void) {
    vec3 color = texture(uSampler, vec2(vUv0.s, vUv0.t)).rgb;
    fragColor = vec4(color, 1.0);
}
]]
    local program = GLSLANG.newProgram()
    local vsShader = GLSLANG.newShader(GLSLANG.EShLangVertex)
    vsShader:setString(DefaultVertexShader)
    vsShader:setEnvInput(GLSLANG.EShSourceGlsl, GLSLANG.EShLangVertex, GLSLANG.EShClientOpenGL, 100)
    vsShader:setEnvClient(GLSLANG.EShClientOpenGL, GLSLANG.EShTargetOpenGL_450)
    vsShader:setEnvTarget(GLSLANG.EShTargetSpv, GLSLANG.EShTargetSpv_1_0)
    vsShader:setAutoMapLocations(true)
    if not vsShader:parse(100, true, 0) then
        error(vsShader:getInfoLog())
    end
    program:addShader(vsShader)

    local fsShader = GLSLANG.newShader(GLSLANG.EShLangFragment)
    fsShader:setString(DefaultFragmentShader)
    fsShader:setEnvInput(GLSLANG.EShSourceGlsl, GLSLANG.EShLangFragment, GLSLANG.EShClientOpenGL, 100)
    fsShader:setEnvClient(GLSLANG.EShClientOpenGL, GLSLANG.EShTargetOpenGL_450)
    fsShader:setEnvTarget(GLSLANG.EShTargetSpv, GLSLANG.EShTargetSpv_1_0)
    fsShader:setAutoMapLocations(true)
    if not fsShader:parse(100, true, 0) then
        error(fsShader:getInfoLog())
    end
    program:addShader(fsShader)

    if not program:link(0) then
        error(program:getInfoLog())
    end

    local vsIntermediate = program:getIntermediate(GLSLANG.EShLangVertex)
    local vsSpv = GLSLANG.glslangToSpv(program, vsIntermediate)

    local fsIntermediate = program:getIntermediate(GLSLANG.EShLangFragment)
    local fsSpv = GLSLANG.glslangToSpv(program, fsIntermediate)

    if utils.isEmscripten() then
        return SPV_CROSS.compile(vsSpv, { es = true, version = 300 }),
            SPV_CROSS.compile(fsSpv, { es = true, version = 300 })
    end

    return SPV_CROSS.compile(vsSpv, { es = false, version = 420 }),
        SPV_CROSS.compile(fsSpv, { es = false, version = 420 });
end

local function collectEvents()
    local events = {}
    while true do
        local event = SDL.PollEvent()
        if event == nil then
            break
        end
        table.insert(events, event)
    end

    return events
end

GLSLANG.initializeProcess()

if SDL.Init(SDL.INIT_VIDEO | SDL.INIT_TIMER) ~= 0 then
    error(SDL.GetError())
end

local windowWidth = 1280
local windowHeight = 720

local window = SDL.CreateWindow("hello",
    SDL.WINDOWPOS_UNDEFINED,
    SDL.WINDOWPOS_UNDEFINED,
    windowWidth, windowHeight,
    SDL.WINDOW_OPENGL)
local renderer = SDL.CreateRenderer(window, -1, SDL.RENDERER_ACCELERATED)

if not utils.isEmscripten() then
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, SDL.GL_CONTEXT_PROFILE_CORE)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0)
end

local context = SDL.GL_CreateContext(window)
if context == nil then
    error("SDL.GL_CreateContext: " .. SDL.GetError())
end
SDL.GL_MakeCurrent(window, context)
SDL.GL_SetSwapInterval(1)

if not utils.isEmscripten() then
    GL.loadGLLoader()
end

--- @type string[]
local points = {}
for _, v in ipairs(
    {
        -1.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
        -1.0, -1.0, 0.0,
        1.0, -1.0, 0.0,
    }
) do
    table.insert(points, ("f"):pack(v));
end
local points = table.concat(points);

--- @type string[]
local uv0s = {}
for _, v in ipairs(
    {
        0.0, 1.0,
        1.0, 1.0,
        0.0, 0.0,
        1.0, 0.0,
    }
) do
    table.insert(uv0s, ("f"):pack(v))
end
local uv0s = table.concat(uv0s)

local vbPositions = GL.genBuffer()
GL.bindBuffer(GL.ARRAY_BUFFER, vbPositions)
GL.bufferData(GL.ARRAY_BUFFER, points, GL.STATIC_DRAW)
GL.bindBuffer(GL.ARRAY_BUFFER, 0)

local vbUv0s = GL.genBuffer()
GL.bindBuffer(GL.ARRAY_BUFFER, vbUv0s)
GL.bufferData(GL.ARRAY_BUFFER, uv0s, GL.STATIC_DRAW)
GL.bindBuffer(GL.ARRAY_BUFFER, 0)

---@type string[]
local indices = {}
for _, v in ipairs({ 0, 1, 2, 1, 3, 2 }) do
    table.insert(indices, ("I2"):pack(v))
end
local indices = table.concat(indices)

local ibo = GL.genBuffer()
GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, ibo)
GL.bufferData(GL.ELEMENT_ARRAY_BUFFER, indices, GL.STATIC_DRAW)
GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, 0)

local vao = GL.genVertexArray()
GL.bindVertexArray(vao)
GL.bindBuffer(GL.ARRAY_BUFFER, vbPositions)
GL.enableVertexAttribArray(0)
GL.vertexAttribPointer(0, 3, GL.FLOAT, GL.FALSE, 0)
GL.bindBuffer(GL.ARRAY_BUFFER, vbUv0s)
GL.enableVertexAttribArray(1)
GL.vertexAttribPointer(1, 2, GL.FLOAT, GL.FALSE, 0)
GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, ibo)
GL.bindVertexArray(0)

local image = SDL_image.load("./uv_checker.png")
image:lock()
image:flipVertical()
image:unlock()
local info = image:getInfo();
print("<image>\n" .. inspect(info));
if info.format.format == SDL.PIXELFORMAT_ABGR8888 then
    print("image is PIXELFORMAT_ABGR8888")
end

local texImage = GL.genTexture()
GL.bindTexture(GL.TEXTURE_2D, texImage)
GL.pixelStorei(GL.UNPACK_ALIGNMENT, 1)
GL.texImage2D(GL.TEXTURE_2D, 0, GL.RGBA, info.w, info.h, 0, GL.RGBA, GL.UNSIGNED_BYTE, image);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_S, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_T, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MIN_FILTER, GL.LINEAR);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MAG_FILTER, GL.LINEAR);

local vsSource, fsSource = transpileShaders()

local vs = GL.createShader(GL.VERTEX_SHADER)
GL.shaderSource(vs, vsSource)
GL.compileShader(vs)
if GL.getShaderiv(vs, GL.COMPILE_STATUS) ~= GL.TRUE then
    error(GL.getShaderInfoLog(vs))
end

local fs = GL.createShader(GL.FRAGMENT_SHADER)
GL.shaderSource(fs, fsSource)
GL.compileShader(fs)
if GL.getShaderiv(fs, GL.COMPILE_STATUS) ~= GL.TRUE then
    error(GL.getShaderInfoLog(fs))
end

local program = GL.createProgram()
GL.attachShader(program, vs)
GL.attachShader(program, fs)
GL.linkProgram(program)
if GL.getProgramiv(program, GL.LINK_STATUS) ~= GL.TRUE then
    error(GL.getProgramInfoLog(program))
end

local bufferWidth = windowWidth
local bufferHeight = windowHeight

local color = ("B"):pack(0xff):rep(4 * bufferWidth * bufferHeight)

local framebuffer = GL.genFramebuffer();
GL.bindFramebuffer(GL.FRAMEBUFFER, framebuffer);

local renderbuffer = GL.genRenderbuffer();
GL.bindRenderbuffer(GL.RENDERBUFFER, renderbuffer);
GL.renderbufferStorage(GL.RENDERBUFFER, GL.DEPTH_COMPONENT16, bufferWidth, bufferHeight);

local texBackBuffer = GL.genTexture()
GL.bindTexture(GL.TEXTURE_2D, texBackBuffer)
GL.pixelStorei(GL.UNPACK_ALIGNMENT, 1)
GL.texImage2D(GL.TEXTURE_2D, 0, GL.RGBA, bufferWidth, bufferHeight, 0, GL.RGBA, GL.UNSIGNED_BYTE, color);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_S, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_T, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MIN_FILTER, GL.LINEAR);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MAG_FILTER, GL.LINEAR);
GL.framebufferTexture2D(GL.FRAMEBUFFER, GL.COLOR_ATTACHMENT0, GL.TEXTURE_2D, texBackBuffer, 0);

GL.bindRenderbuffer(GL.RENDERBUFFER, renderbuffer);
GL.bindFramebuffer(GL.FRAMEBUFFER, framebuffer);
GL.bindTexture(GL.TEXTURE_2D, texBackBuffer);

local function update()
    local events = collectEvents()
    for _, ev in ipairs(events) do
        if ev.type == SDL.QUIT then
            utils.unregisterFunction("update")
            SDL.Quit()
            return
        end

        print("event = " .. inspect(ev))
    end


    GL.bindFramebuffer(GL.FRAMEBUFFER, framebuffer);
    GL.viewport(0, 0, bufferWidth, bufferHeight)
    local clearColor = { 0.5, 0.5, 0.5, 1.0 }
    GL.clearColor(table.unpack(clearColor))
    GL.clear(GL.COLOR_BUFFER_BIT | GL.DEPTH_BUFFER_BIT)
    GL.useProgram(program)
    GL.bindVertexArray(vao);
    GL.activateTexture(GL.TEXTURE0);
    GL.bindTexture(GL.TEXTURE_2D, texImage);
    GL.uniform1i(0, 0);
    GL.drawElements(GL.TRIANGLES, #indices / 2, GL.UNSIGNED_SHORT)
    GL.bindVertexArray(0);

    GL.bindFramebuffer(GL.FRAMEBUFFER, 0);
    GL.viewport(0, 0, windowWidth, windowHeight)
    GL.useProgram(program)
    GL.bindVertexArray(vao);
    GL.activateTexture(GL.TEXTURE0);
    GL.bindTexture(GL.TEXTURE_2D, texBackBuffer);
    GL.uniform1i(0, 0);
    GL.drawElements(GL.TRIANGLES, #indices / 2, GL.UNSIGNED_SHORT)
    GL.bindVertexArray(0);

    SDL.GL_SwapWindow(window)
end

local function finalize()
    GL.deleteRenderbuffer(renderbuffer);
    GL.deleteFramebuffer(framebuffer);
    GL.deleteTexture(texBackBuffer);
    GL.deleteTexture(texImage);
    GLSLANG.finalizeProcess()
end

utils.registerFunction("update", function(...)
    return handleError(pcall(update, ...))
end)

utils.registerFunction("finalize", function(...)
    return handleError(pcall(finalize, ...))
end)
