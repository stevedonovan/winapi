require 'winapi'

l = 1
winapi.timer(400,function()
  print 'bonzo'
  l = l + 1
  if l > 10 then os.exit() end
end)

k = 1
winapi.timer(300,function()
  print 'alice'
  k = k  +1
  if k > 5 then return true end
end)

winapi.sleep(-1)
