require 'winapi'
t = os.clock()
local P = {}
P[1] = winapi.spawn 'lua slow.lua'
P[2] = winapi.spawn 'lua slow.lua'
winapi.wait_for_processes(P,true)
print(os.clock() - t)

