local W = require 'winapi'
W.set_encoding(W.CP_UTF8)
console = W.get_foreground_window()
console:set_text 'ελληνική'
W.set_clipboard 'ελληνική'
io.read()

