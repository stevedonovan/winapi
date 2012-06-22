require 'winapi'
io.stdout:setvbuf 'no'
local t1,t2
t1 = winapi.make_timer(500,function()
  print 'gotcha'
end)

local k = 1
t2 = winapi.make_timer(400,function()
  k = k + 1
  print(k)
  if k > 5 then
    print 'killing'
    t1:kill() -- kill the first timer
    t2 = nil
    return true -- and we will end now
   end
end)

winapi.make_timer(1000,function()
  print 'doo'
  if not t2 then os.exit(0) end -- we all finish
end)

-- sleep forever...
winapi.sleep(-1)
