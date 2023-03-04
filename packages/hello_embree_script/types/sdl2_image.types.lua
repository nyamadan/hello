--- @class SDL_image
--- @field load fun(file: string): SDL_Surface

--- @class SDL_Surface_Info_Format
--- @field BitsPerPixel integer
--- @field BytePerPixel integer
--- @field format integer

--- @class SDL_Surface_Info
--- @field w integer
--- @field h integer
--- @field pitch integer
--- @field format SDL_Surface_Info_Format

--- @class SDL_Surface
--- @field getInfo fun(self: SDL_Surface): SDL_Surface_Info
--- @field free fun(self: SDL_Surface)
--- @field lock fun(self: SDL_Surface)
--- @field unlock fun(self: SDL_Surface)
--- @field flipVertical fun(self: SDL_Surface)
