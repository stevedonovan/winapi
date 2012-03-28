require 'winapi'

--[[ -- blocking version
winapi.make_pipe_server(function(f)
    local res = f:read()
    f:write(res:upper())
end)
-- ]]

--[[ 'node.js' style
winapi.make_pipe_server(function(f)
    f:read_async(function(res)
        f:write(res:upper())
    end)
end)
--]]

local wrap, yield, resume = coroutine.wrap, coroutine.yield, coroutine.resume

--[[
winapi.make_pipe_server(function(f)
    local fun = function(f)
        while true do
            local res = f:read()
            if res == 'close' then break end
            f:write(res:upper())
        end
    end
    local co = coroutine.create(fun)
    resume(co,fwrap(f,co))
end)
]]

--~             f:read_async(function(txt)
--~                resume(co,txt)
--~             end)

function fwrap (f,co)
    local obj = {}
    local started
    function obj:read ()
        if not started then
            f:read_async(co)
            started = true
        end
        return yield()
    end
    function obj:write (s)
        return f:write(s)
    end
    return obj
end

function winapi.make_pipe_server_async(fun)
    winapi.make_pipe_server(function(f)
        local co = coroutine.wrap(fun)
        co(fwrap(f,co))
    end)
end

winapi.make_pipe_server_async(function(f)
    while true do
        local res = f:read()
        if res == 'close' then break end
        f:write(res:upper())
    end
    print 'finis'
end)


winapi.sleep(-1)

