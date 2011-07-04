require 'winapi'
t = os.clock()
n = tonumber(arg[1] or 2)
local P = {}
for i = 1,n do
    P[i],f = winapi.spawn_process ('lua slow.lua '..i)
    f:read_async(print)
end
winapi.wait_for_processes(P,true)
print(os.clock() - t)

