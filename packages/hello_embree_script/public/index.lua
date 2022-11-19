local function run(f, err)
    if err ~= nil then
        local message = "Error: " .. err
        print(message)
        error(message)
    end
    return f
end
run(pcall(run(loadfile("./main.lua"))))
