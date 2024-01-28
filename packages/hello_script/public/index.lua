local handleError = require("handle_error")
local main = handleError(loadfile("main.lua"))
if main == nil then
    error("main function is nil")
end
handleError(pcall(main))
