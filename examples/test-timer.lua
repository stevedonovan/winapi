require 'winapi'
io.stdout:setvbuf 'no'
winapi.timer(500,function()
  print 'gotcha'
end)
local k = 1
winapi.timer(400,function()
  print 'dolly'
  k = k + 1
  if k > 5 then return true end
end)
winapi.sleep(-1)
