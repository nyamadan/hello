---@generic T : any
---@param result T
---@param message any
---@param ... any
---@return T
---@return any
---@return any
local function handleError(result, message, ...)
    if not result then
        print("Error: " .. message)
        error(message)
    end
    return result, message, ...
end

return handleError
