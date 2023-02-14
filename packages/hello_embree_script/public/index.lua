local handleError = require("handle_error")
local main = handleError(loadfile("main.lua"))
handleError(pcall(main))
