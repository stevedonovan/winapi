-- this shows the new wait_for_input_idle, so there's no need for
-- a random wait until an application is ready to go.
-- Note, if we use spawn() then the window is initially invisible,
-- and needs to be shown explicitly.
require 'winapi'
P = winapi.spawn_process 'notepad'
P:wait_for_input_idle()
w = winapi.find_window_match 'Untitled'
w:show()
w:set_foreground()
winapi.send_to_window 'hello dammit'

