require 'winapi'

function printer(msec,msg)
  local i = 1
  return winapi.make_timer(500,function()
    print (msg,i)
    i = i + 1
  end)
end

printer(500,'bob')
printer(500,'june')
printer(500,'alice')
printer(500,'jim')

winapi.sleep(-1)
