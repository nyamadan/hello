function protect(tbl)
    return setmetatable({}, {
        __index = tbl,
        __newindex = function(t, key, value)
            error("attempting to change constant " ..
                   tostring(key) .. " to " .. tostring(value), 2)
        end
    })
end

function dump(o)
    if type(o) == 'table' then
        local s = '{ '
        for k,v in pairs(o) do
                if type(k) ~= 'number' then k = '"'..k..'"' end
                s = s .. '['..k..'] = ' .. dump(v) .. ','
        end
        return s .. '} '
    else
        return tostring(o)
    end
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
    {
        materialType = MaterialType.REFRACTION,
        baseColorFactor = {1.0, 1.0, 1.0, 1.0},
        emissiveFactor = {0.0, 0.0, 0.0},
        metalnessFactor = 1.0,
        roughnessFactor = 0.0
    },
    {
        translate = {-3, 1, 0},
        scale = {1, 1, 1},
        rotate = {0, 0, 0, 1}
    },
    80, 60
)

_loadSphere(
    {
        materialType = MaterialType.REFLECTION,
        baseColorFactor = {1.0, 0.0, 0.0, 1.0},
        emissiveFactor = {0.0, 0.0, 0.0},
        metalnessFactor = 1.0,
        roughnessFactor = 0.25
    },
    {
        translate = {3, 1, 0},
        scale = {1, 1, 1},
        rotate = {0, 0, 0, 1}
    },
    80, 60
)

_loadPlane(
    {
        materialType = MaterialType.REFLECTION,
        baseColorFactor = {0.2, 0.2, 0.2, 1.0},
        emissiveFactor = {0.0, 0.0, 0.0},
        metalnessFactor = 0.0,
        roughnessFactor = 0.0
    },
    {
        translate = {0, 0, 0},
        scale = {10, 10, 10},
        rotate = {0, 0, 0, 1}
    }
)

local geoms, err = _loadModel(
    "../glTF-Sample-Models/2.0/DamagedHelmet/glTF-Binary/DamagedHelmet.glb",
    {
        translate = {0, 1, 0},
        scale = {1, 1, 1},
        rotate = {0, 0, 0, 1},
    }
)

print(dump(geoms))

for i, v in ipairs(geoms) do
    local mat = _getGeometryMaterial(v)
    print(dump(mat))

    local ok = _replaceGeometryPrimitiveMaterial(v, {
        materialType = MaterialType.REFRACTION,
        baseColorFactor = {1.0, 1.0, 1.0, 1.0},
        emissiveFactor = {0.0, 0.0, 0.0},
        metalnessFactor = 1.0,
        roughnessFactor = 0.0
    })

    print(ok)

    _commitGeometry(v)
end

_commitScene()

_setRenderMode(RenderMode.PATHTRACING)
_setMaxSamples(20)

local f = io.open("out/video.y4m", "wb")

f:write("YUV4MPEG2 W" .. _getImageWidth() .. " H" .. _getImageHeight()
    .. " F30000:1001 Ip A0:0 C420 XYSCSS=420\n")

for i=1,60 do
    print("FRAME: " .. i)

    local running = true

    _setLensRadius(0.75)
    _setFocusDistance(i * 0.10 + 2.0)
    _reset()

    while running do
        if _render() then
            _denoise()
            running = false
        end

        _finish(true, not running);

        coroutine.yield()
    end

    f:write("FRAME\n")
    _writeFrameYUV420(f)
end


f:close()