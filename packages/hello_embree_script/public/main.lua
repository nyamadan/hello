local utils = require("utils")
--- @type buffer
local Buffer = require("buffer")
--- @type gl
local GL = require("opengl")
local SDL = require("sdl2")
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
void main(void) {
    gl_Position = vec4(aPosition, 1.0);
}
]]

    local DefaultFragmentShader = [[#version 310 es
precision mediump float;

// layout(location=0) uniform vec2 resolution;
// layout(location=1) uniform sampler2D backbuffer;

layout(location=0) out vec4 fragColor;

void main(void) {
    // vec2 uv = gl_FragCoord.xy / resolution;
    // vec3 color = texture(backbuffer, uv).rgb;
    vec3 color = vec3(1.0, 0.0, 1.0);
    fragColor = vec4(color, 1.0);
}
]]
    local program = GLSLANG.newProgram()
    local vsShader = GLSLANG.newShader(GLSLANG.EShLangVertex)
    vsShader:setString(DefaultVertexShader)
    vsShader:setEnvInput(GLSLANG.EShSourceGlsl, GLSLANG.EShLangVertex, GLSLANG.EShClientOpenGL, 100)
    vsShader:setEnvClient(GLSLANG.EShClientOpenGL, GLSLANG.EShTargetOpenGL_450)
    vsShader:setEnvTarget(GLSLANG.EShTargetSpv, GLSLANG.EShTargetSpv_1_0)
    if not vsShader:parse(100, true, 0) then
        error(vsShader:getInfoLog())
    end
    program:addShader(vsShader)

    local fsShader = GLSLANG.newShader(GLSLANG.EShLangFragment)
    fsShader:setString(DefaultFragmentShader)
    fsShader:setEnvInput(GLSLANG.EShSourceGlsl, GLSLANG.EShLangFragment, GLSLANG.EShClientOpenGL, 100)
    fsShader:setEnvClient(GLSLANG.EShClientOpenGL, GLSLANG.EShTargetOpenGL_450)
    fsShader:setEnvTarget(GLSLANG.EShTargetSpv, GLSLANG.EShTargetSpv_1_0)
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

local window = SDL.CreateWindow("hello embree",
    SDL.WINDOWPOS_UNDEFINED,
    SDL.WINDOWPOS_UNDEFINED,
    1280, 720,
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

---@type number[]
local points = { -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 0.0, 1.0, 0.0 }
local buffer = Buffer.alloc(#points * 4)
for i, v in ipairs(points) do
    buffer:setFloat32(4 * (i - 1), v)
end

local vbo = GL.genBuffer()
GL.bindBuffer(GL.ARRAY_BUFFER, vbo)
GL.bufferData(GL.ARRAY_BUFFER, buffer, GL.STATIC_DRAW)
GL.bindBuffer(GL.ARRAY_BUFFER, 0)

local vao = GL.genVertexArray()
GL.bindVertexArray(vao)
GL.bindBuffer(GL.ARRAY_BUFFER, vbo)
GL.enableVertexAttribArray(0)
GL.vertexAttribPointer(0, 3, GL.FLOAT, GL.FALSE, 0)
GL.bindVertexArray(0)

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

local color = Buffer.alloc(512 * 512 * 4)
color:fillUint8(0xff)

local texColor = GL.genTexture()
GL.bindTexture(GL.TEXTURE_2D, texColor)
GL.pixelStorei(GL.UNPACK_ALIGNMENT, 1)
GL.texImage2D(GL.TEXTURE_2D, 0, GL.RGBA, 512, 512, 0, GL.RGBA, GL.UNSIGNED_BYTE, color);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_S, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_WRAP_T, GL.CLAMP_TO_EDGE);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MIN_FILTER, GL.LINEAR);
GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MAG_FILTER, GL.LINEAR);
GL.bindTexture(GL.TEXTURE_2D, 0);

local rbo = GL.genRenderbuffer();
GL.bindRenderbuffer(GL.RENDERBUFFER, rbo);
GL.renderbufferStorage(GL.RENDERBUFFER, GL.DEPTH_COMPONENT, 512, 512);
GL.bindRenderbuffer(GL.RENDERBUFFER, 0);
GL.deleteRenderbuffer(rbo);

local fbo = GL.genFramebuffer();
GL.deleteFramebuffer(fbo);

local function update()
    local events = collectEvents()
    for _, ev in ipairs(events) do
        if ev.type == SDL.QUIT then
            utils.unregisterFunction("update")
            SDL.Quit()
            return
        end

        print("ev: type = " .. ev.type)
    end

    GL.clearColor(0.5, 0.5, 0.5, 1)
    GL.clear(GL.COLOR_BUFFER_BIT | GL.DEPTH_BUFFER_BIT)

    GL.useProgram(program)
    GL.bindVertexArray(vao);
    GL.drawArrays(GL.TRIANGLES, 0, 3)
    GL.bindVertexArray(0);

    SDL.GL_SwapWindow(window)
end

utils.registerFunction("update", function(...)
    return handleError(pcall(update, ...))
end)
