    local W = require 'winapi'
    local e = W.event()
    local count = 1

    W.make_timer(500,function()
        print 'finis'
        if count == 5 then
	    print 'finished!'
            os.exit()
        end
        e:signal()
        count = count + 1
    end)

    while true do
        e:wait()
        print 'gotcha'
    end
