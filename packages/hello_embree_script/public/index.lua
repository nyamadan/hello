local utils = require("utils")

local function handleError(result, err, ...)
    if not result then
        print("Error: " .. err)
        error(err)
    end

    return result, err, ...
end

local function fetch(url)
    local request = utils.fetch(url)

    coroutine.yield()

    while true do
        local result = utils.getFetchRequest(request)

        if result.finished and result.succeeded and result.readyState == 4 and result.data ~= nil then
            return result;
        end

        if result.finished and not result.succeeded then
            return result;
        end

        coroutine.yield()
    end
end

local function lazyRequire(modname)
    if utils.isEmscripten() and modname:sub(1, 1) == "." then
        local path = modname .. ".lua"
        if package.loaded[modname] == nil then
            print("request(start) : " .. path)
            local response = fetch(path)
            local source = response.data:getString()
            local f = io.open(path, "w")
            if f == nil then
                error("Could not write error.")
            end
            f:write(source)
            f:close()
            print("request(done) : " .. path)
        else
            print("request(cached) : " .. path)
        end
    end

    return require(modname)
end

local function lazyStart(start)
    local co = coroutine.create(function()
        return start(lazyRequire)
    end)

    while true do
        local status = coroutine.status(co)
        if status == "suspended" or status == "normal" then
            local _, update = handleError(coroutine.resume(co))
            if coroutine.status(co) == "dead" then
                return update
            end
            coroutine.yield()
        end
    end
end

local main = coroutine.create(function()
    local start = lazyRequire("./main")
    local update = lazyStart(start)
    while true do
        handleError(pcall(update))
        coroutine.yield()
    end
end)

local function update()
    local status = coroutine.status(main)
    if status == "suspended" or status == "normal" then
        handleError(coroutine.resume(main))
    end
end

utils.registerFunction("update", update)
