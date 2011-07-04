require 'winapi'
P,W = winapi.spawn_process 'lua test-timer.lua'
stuff = W:read()
k = 1
while stuff do
 io.write(stuff);
 stuff = W:read()
 k = k + 1
 if k > 15 then P:kill() end
end
