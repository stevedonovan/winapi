require 'winapi'
local dir = 'd:\\dev\\lua\\LuaMacro\\tests'
local dir2 = dir .. '\\test'
local LAST_WRITE,FILE_NAME =
        winapi.FILE_NOTIFY_CHANGE_LAST_WRITE,
        winapi.FILE_NOTIFY_CHANGE_FILE_NAME

ok,err = winapi.watch_for_file_changes(dir,LAST_WRITE+FILE_NAME,TRUE,print)
if not ok then return print(err) end
ok,err = winapi.watch_for_file_changes(dir2,LAST_WRITE+FILE_NAME,TRUE,print)
if not ok then return print(err) end

winapi.sleep(-1)
