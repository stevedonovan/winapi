
local W = require 'winapi'

require 'message_constants'

W.beep(0)
W.sleep(300)
W.beep(MB.ICONWARNING)
W.sleep(500)
W.beep(MB.ICONERROR)



W.show_message("Message","stuff",MB.YESNO + MB.ICONWARNING)
--W.show_message("Message","stuff",MB.OK + MB.ICONQUESTION)



