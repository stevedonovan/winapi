local W = require 'winapi'
local r,w = W.pipe()
local m = W.mutex()

function lprint(...)
  m:lock()
  print(...)
  m:release()
end

function long(name)
   lprint('hello',name)
   for i = 1,2 do
     m:lock()
     w:write(name..i)
     m:release()
     for i = 1,1e8 do end
   end
end

r:read_async(function(s)
  lprint(s)
end)

T = {}
T[1] = W.thread(long,'john')
T[2] = W.thread(long,'jane')
T[3] = W.thread(long,'june')

W.wait_for_processes(T,true)
print 'finish'

