print "This is main.lua"

local ok, message = pcall(function()
    local utils = require("utils")
    local SDL = require("sdl2")

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
