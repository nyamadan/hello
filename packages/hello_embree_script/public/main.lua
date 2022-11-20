local utils = require("utils")
local buffer = require("buffer")
local SDL = require("sdl2")
local gl = require("opengl")
local glslang = require("glslang")
local spv_cross = require("spv_cross")

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
    local program = glslang.newProgram()
    local vsShader = glslang.newShader(glslang.EShLangVertex)
    vsShader:setString(DefaultVertexShader)
    vsShader:setEnvInput(glslang.EShSourceGlsl, glslang.EShLangVertex, glslang.EShClientOpenGL, 100)
    vsShader:setEnvClient(glslang.EShClientOpenGL, glslang.EShTargetOpenGL_450)
    vsShader:setEnvTarget(glslang.EShTargetSpv, glslang.EShTargetSpv_1_0)
    if not vsShader:parse(100, true, 0) then
        error(vsShader:getInfoLog())
    end
    program:addShader(vsShader)

    local fsShader = glslang.newShader(glslang.EShLangFragment)
    fsShader:setString(DefaultFragmentShader)
    fsShader:setEnvInput(glslang.EShSourceGlsl, glslang.EShLangFragment, glslang.EShClientOpenGL, 100)
    fsShader:setEnvClient(glslang.EShClientOpenGL, glslang.EShTargetOpenGL_450)
    fsShader:setEnvTarget(glslang.EShTargetSpv, glslang.EShTargetSpv_1_0)
    if not fsShader:parse(100, true, 0) then
        error(fsShader:getInfoLog())
    end
    program:addShader(fsShader)

    if not program:link(0) then
        error(program:getInfoLog())
    end

    local vsIntermediate = program:getIntermediate(glslang.EShLangVertex)
    local vsSpv = glslang.glslangToSpv(program, vsIntermediate)

    local fsIntermediate = program:getIntermediate(glslang.EShLangFragment)
    local fsSpv = glslang.glslangToSpv(program, fsIntermediate)

    if utils.isEmscripten() then
        return spv_cross.compile(vsSpv, { es = true, version = 300 }),
            spv_cross.compile(fsSpv, { es = true, version = 300 })

    end

    return spv_cross.compile(vsSpv, { es = false, version = 420 }),
        spv_cross.compile(fsSpv, { es = false, version = 420 });
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

glslang.initializeProcess()

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
    gl.loadGLLoader()
end

---@type number[]
local points = { -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 0.0, 1.0, 0.0 }
local buffer = buffer.alloc(#points * 4)
for i, v in ipairs(points) do
    buffer:setFloat32(4 * (i - 1), v)
end

local vbo = gl.genBuffer()
gl.bindBuffer(gl.ARRAY_BUFFER, vbo)
gl.bufferData(gl.ARRAY_BUFFER, buffer, gl.STATIC_DRAW)
gl.bindBuffer(gl.ARRAY_BUFFER, 0)

local vao = gl.genVertexArray()
gl.bindVertexArray(vao)
gl.bindBuffer(gl.ARRAY_BUFFER, vbo)
gl.enableVertexAttribArray(0)
gl.vertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 0)
gl.bindVertexArray(0)

local vsSource, fsSource = transpileShaders()

local vs = gl.createShader(gl.VERTEX_SHADER)
gl.shaderSource(vs, vsSource)
gl.compileShader(vs)
if gl.getShaderiv(vs, gl.COMPILE_STATUS) ~= gl.TRUE then
    error(gl.getShaderInfoLog(vs))
end

local fs = gl.createShader(gl.FRAGMENT_SHADER)
gl.shaderSource(fs, fsSource)
gl.compileShader(fs)
if gl.getShaderiv(fs, gl.COMPILE_STATUS) ~= gl.TRUE then
    error(gl.getShaderInfoLog(fs))
end

local program = gl.createProgram()
gl.attachShader(program, vs)
gl.attachShader(program, fs)
gl.linkProgram(program)
if gl.getProgramiv(program, gl.LINK_STATUS) ~= gl.TRUE then
    error(gl.getProgramInfoLog(program))
end

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

    gl.clearColor(0.5, 0.5, 0.5, 1)
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

    gl.useProgram(program)
    gl.bindVertexArray(vao);
    gl.drawArrays(gl.TRIANGLES, 0, 3)
    gl.bindVertexArray(0);

    SDL.GL_SwapWindow(window)
end

utils.registerFunction("update", function(...)
    return handleError(pcall(update, ...))
end)
