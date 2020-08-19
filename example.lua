function protect(tbl)
    return setmetatable({}, {
        __index = tbl,
        __newindex = function(t, key, value)
            error("attempting to change constant " ..
                   tostring(key) .. " to " .. tostring(value), 2)
        end
    })
end

local RenderMode = protect({
    ALBEDO = 0,
    EMISSIVE = 1,
    NORMAL = 2,
    TANGENT = 3,
    BITANGENT = 4,
    PATHTRACING = 5
})

local MaterialType = protect({
    REFLECTION = 0,
    REFRACTION = 1
})

_loadSphere(
    MaterialType.REFRACTION,    -- Type
    1.0, 1.0, 1.0, 1.0,         -- baseColor
    nil,                        -- baseColorTexture
    nil,                        -- normalTexture
    0.0,                       -- roughness
    1.0,                        -- metalness
    nil,                        -- roughnessMetalnessTexture
    0.0, 0.0, 0.0,              -- emissive
    nil,                        -- emissiveTexture

    80, 60,                     -- segments

    -3, 1, 0,                    -- position
    1, 1, 1,                    -- scale
    0, 0, 0, 1                  -- quat
)

_loadSphere(
    MaterialType.REFLECTION,    -- Type
    1.0, 0.0, 0.0, 1.0,         -- baseColor
    nil,                        -- baseColorTexture
    nil,                        -- normalTexture
    0.25,                       -- roughness
    1.0,                        -- metalness
    nil,                        -- roughnessMetalnessTexture
    0.0, 0.0, 0.0,              -- emissive
    nil,                        -- emissiveTexture

    80, 60,                     -- segments

    3, 1, 0,                    -- position
    1, 1, 1,                    -- scale
    0, 0, 0, 1                  -- quat
)

_loadPlane(
    MaterialType.REFLECTION,    -- Type
    0.2, 0.2, 0.2, 1.0,         -- baseColor
    nil,                        -- baseColorTexture
    nil,                        -- normalTexture
    0.0,                       -- roughness
    0.0,                        -- metalness
    nil,                        -- roughnessMetalnessTexture
    0.0, 0.0, 0.0,              -- emissive
    nil,                        -- emissiveTexture

    0, 0, 0,                    -- position
    10, 10, 10,                    -- scale
    0, 0, 0, 1                  -- quat
)

_commitScene()

local running = true

_setRenderMode(RenderMode.PATHTRACING)
_setMaxSamples(100)

local f = io.open("test.y4m", "wb")

f:write("YUV4MPEG2 W" .. _getImageWidth() .. " H" .. _getImageHeight() .. " F30000:1001 Ip A0:0 C420 XYSCSS=420\n")

while running do
    if _render() then
        _denoise()
        running = false
    end

    _finish(true, true);

    f:write("FRAME\n")
    _writeFrameYUV420(f)

    coroutine.yield()
end

f:close()