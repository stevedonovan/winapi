require 'winapi'
io.stdout:setvbuf 'no'
local t1,t2
t1 = winapi.timer(500,function()
  print 'gotcha'
end)
local k = 1
t2 = winapi.timer(400,function()
  print (t1)
  k = k + 1
  if k > 5 then
    t1:kill() -- kill the first timer
    t2 = nil
    return true -- and we will end now
   end
end)
winapi.timer(1000,function()
  print 'doo'
  if not t2 then os.exit(0) end -- we all finish
end)
winapi.sleep(-1)
