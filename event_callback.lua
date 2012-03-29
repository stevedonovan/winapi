require 'winapi'

e = winapi.event()

winapi.make_timer(500,function()
  print 'signal'
  e:signal()
end)

--[[
while true do
  e:wait()
  print 'ok'
end
--]]

e:wait_async(function(s)
  print ('finis',s)
  os.exit()
end)

--print 'sleeping'

winapi.sleep(-1)
