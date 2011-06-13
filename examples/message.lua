
local W = require 'winapi'

--[[
W.beep()
W.sleep(300)
W.beep 'warning'
W.sleep(500)
W.beep 'error'
]]



print(W.show_message("Message","stuff"))
print(W.show_message("Message","stuff\nand nonsense","yes-no","warning"))




