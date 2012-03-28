require 'winapi'

local e = winapi.event();

winapi.make_timer(500,function()
    print 'finis'
    e:signal()
end)

while true do
    e:wait()
    print 'gotcha'
end

