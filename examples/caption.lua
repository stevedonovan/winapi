local W = require 'winapi'
local console = W.get_foreground_window()
console:set_text 'e???????'
W.set_clipboard 'e???????'
print 'Press enter'
io.read()
