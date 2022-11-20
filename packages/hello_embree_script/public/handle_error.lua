---@generic T : any
---@param result T
---@param message any
---@param ... any
---@return T
---@return any
---@return any
local function handleError(result, message, ...)
    if not result then
        local err = debug.traceback(message, 2)
        print("Error: " .. err)
        error(err)
    end
    return result, message, ...
end

return handleError
