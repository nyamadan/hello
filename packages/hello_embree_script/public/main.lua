print "This is main.lua"

local ok, message = pcall(function()
    local utils = require("utils")
    local buffer = require("buffer")
    local SDL = require("sdl2")
    local gl = require("opengl")
    local glslang = require("glslang")
    local spv_cross = require("spv_cross")

    local DefaultVertexShader = [[#version 310 es
layout(location=0) in vec3 aPosition;
void main(void) {
    gl_Position = vec4(aPosition, 1.0);
}
]]

local DefaultFragmentShader = [[#version 310 es
precision mediump float;

layout(location=0) uniform vec2 resolution;
layout(location=1) uniform sampler2D backbuffer;

layout(location=0) out vec4 fragColor;

void main(void) {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec3 color = texture(backbuffer, uv).rgb;
    fragColor = vec4(color, 1.0);
}
]]

    glslang.initializeProcess();

    local program = glslang.newProgram();

    local vsShader = glslang.newShader(glslang.EShLangVertex);
    vsShader:setString(DefaultVertexShader);
    vsShader:setEnvInput(glslang.EShSourceGlsl, glslang.EShLangVertex, glslang.EShClientOpenGL, 100);
    vsShader:setEnvClient(glslang.EShClientOpenGL, glslang.EShTargetOpenGL_450);
    vsShader:setEnvTarget(glslang.EShTargetSpv, glslang.EShTargetSpv_1_0);
    if(not vsShader:parse(100, true, 0)) then
        error(vsShader:getInfoLog())
    end
    program:addShader(vsShader);

    local fsShader = glslang.newShader(glslang.EShLangFragment);
    fsShader:setString(DefaultFragmentShader);
    fsShader:setEnvInput(glslang.EShSourceGlsl, glslang.EShLangFragment, glslang.EShClientOpenGL, 100);
    fsShader:setEnvClient(glslang.EShClientOpenGL, glslang.EShTargetOpenGL_450);
    fsShader:setEnvTarget(glslang.EShTargetSpv, glslang.EShTargetSpv_1_0);
    if(not fsShader:parse(100, true, 0)) then
        error(fsShader:getInfoLog())
    end
    program:addShader(fsShader);

    if not program:link(0) then
        error(program:getInfoLog())
    end

    local vsIntermediate = program:getIntermediate(glslang.EShLangVertex);
    local vsSpv = glslang.glslangToSpv(program, vsIntermediate);

    local fsIntermediate = program:getIntermediate(glslang.EShLangFragment);
    local fsSpv = glslang.glslangToSpv(program, fsIntermediate);

    if utils.isEmscripten() then
        print(spv_cross.compile(vsSpv, {es = true, version = 300}));
        print(spv_cross.compile(fsSpv, {es = true, version = 300}));
    else
        print(spv_cross.compile(vsSpv, {es = false, version = 420}));
        print(spv_cross.compile(fsSpv, {es = false, version = 420}));
    end

    local function collectEvents()
        local events = {};
        while true do
            local event = SDL.PollEvent()
            if event == nil then
                break
            end
            table.insert(events, event)
        end

        return events;
    end

    if SDL.Init(SDL.INIT_VIDEO | SDL.INIT_TIMER) ~= 0 then
        error(SDL.GetError())
    end

    local window = SDL.CreateWindow("Hello World!!", SDL.WINDOWPOS_UNDEFINED, SDL.WINDOWPOS_UNDEFINED, 1280, 720, SDL.WINDOW_OPENGL)
    local renderer = SDL.CreateRenderer(window, -1, SDL.RENDERER_ACCELERATED)

    local GLSL_VERSION = "#version 130"
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, SDL.GL_CONTEXT_PROFILE_CORE)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3)
    SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0)
    SDL.GL_CreateContext(window);
    SDL.GL_SetSwapInterval(1)

    utils.registerFunction("update", function()
        local ok, message = pcall(function()
            local events = collectEvents()
            for _, ev in ipairs(events) do
                if ev.type == SDL.QUIT then
                    utils.unregisterFunction("update")
                    SDL.Quit()
                    return
                end

                print("ev: type = " .. ev.type)
            end

            gl.ClearColor(1, 0, 1, 1)
            gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

            SDL.GL_SwapWindow(window)
        end)
        if not ok then
            print(message)
        end
    end)
end)

if not ok then
    print(message)
end
