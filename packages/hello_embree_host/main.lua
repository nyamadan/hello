print "This is main.lua"

local ok, message = pcall(function()
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

    if SDL.Init(SDL.INIT_VIDEO) ~= 0 then
        error(SDL.GetError())
    end

    local window = SDL.CreateWindow("Hello World!!", SDL.WINDOWPOS_UNDEFINED, SDL.WINDOWPOS_UNDEFINED, 1280, 720, 0)
    local renderer = SDL.CreateRenderer(window, -1, SDL.RENDERER_ACCELERATED)

    local running = true
    while running do
        local events = collectEvents()
        for _, ev in ipairs(events) do
            if ev.type == SDL.QUIT then
                running = false
            end
        end
        SDL.Delay(16)
    end

    SDL.Quit()
end)

if not ok then
    print(message)
end
