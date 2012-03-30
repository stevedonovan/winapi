require 'winapi'

fprintf = require 'pl.utils'.fprintf
stderr = io.stderr

e = winapi.event()

winapi.make_timer(500,function()
  fprintf(stderr,'signal!\n')
  e:signal()
end)

--[[
while true do
  e:wait()
  print 'ok'
end
--]]

e:wait_async(function(s)
  fprintf (stderr,'finis %s\n',s)
  os.exit()
end)

fprintf(stderr,'sleeping\n')

winapi.sleep(-1)
