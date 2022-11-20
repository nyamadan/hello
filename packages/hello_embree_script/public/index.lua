local function handleError(f, err)
    if err ~= nil then
        print(err)
        error("Runtime error occured.")
    end
    return f
end

handleError(pcall(handleError(loadfile("./main.lua"))))
