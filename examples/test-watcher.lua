require 'winapi'
io.stdout:setvbuf 'no'
local dir = '.'
local dir2 = dir .. '\\without_spaces'
local LAST_WRITE,FILE_NAME =
        winapi.FILE_NOTIFY_CHANGE_LAST_WRITE,
        winapi.FILE_NOTIFY_CHANGE_FILE_NAME

w1,err = winapi.watch_for_file_changes(dir,LAST_WRITE+FILE_NAME,false,print)
if not w1 then return print(err) end
w2,err = winapi.watch_for_file_changes(dir2,LAST_WRITE+FILE_NAME,false,print)
if not w2 then return print(err) end

-- ok, our watchers are in the background
winapi.sleep(200)

function writefile (name,text)
    local f = io.open(name,'w')
    f:write(text)
    f:close()
end

writefile('without_spaces/out.txt','hello')

os.execute 'cd without_spaces && del frodo.txt && ren out.txt frodo.txt'

writefile ('mobo.txt','freaky')

winapi.sleep(200)

-- can stop a watcher by killing its thread
w1:kill()

writefile ('doof.txt','freaky')

winapi.sleep(200)





