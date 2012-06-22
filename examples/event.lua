local W = require 'winapi'
local e = W.event()
local count = 1
local finished

W.make_timer(500,function()
    print 'tick'
    if count == 5 then
        print 'finished!'
        finished = true
    end
    e:signal()
    count = count + 1
end)

while not finished do
    e:wait()
    print 'gotcha'
end
