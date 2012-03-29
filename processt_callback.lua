require 'winapi'

-- run the program: 'fire and forget'
p,f = winapi.spawn_process 'lua52 process-wait.lua'

-- echo process output to stdout
f:read_async(print)

-- call this function when we're finished
p:wait_async(function(s)
  print ('finis',s,p:get_exit_code())
  os.exit()
end)

a = 0

--print 'sleeping'

winapi.sleep(-1)
