require 'winapi'

l = 1
winapi.make_timer(400,function()
  print 'bonzo'
  l = l + 1
  --if l > 10 then os.exit() end
end)

winapi.make_timer(300,function()
  print 'woo'
end)

function protected(val)
  print(val)
end

-- --[[
while true do
  winapi.sleep(200,true)  
  print 'gotcha'
end
--]]

--[[
while true do
  winapi.sleep(200)
  winapi.locked_eval(protected,'hello')
  --protected 'hello'  --> this will cause nonsense
end
--]]


