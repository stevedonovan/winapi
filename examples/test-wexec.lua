require 'winapi'

local long = 'ελληνική.txt'
local UTF8 = winapi.CP_UTF8
winapi.set_encoding(UTF8)

function winapi.tmpname () return os.getenv('TEMP')..os.tmpname() end
function winapi.execute_unicode(cmd)
   local tmpfile = winapi.tmpname()
   cmd = os.getenv('COMSPEC')..' /u /c '..cmd..' > "'..tmpfile..'"'
   local P,err = winapi.spawn(cmd)
   if not P then return nil,err end
   local res,f,out
   res = P:wait():exit_code()
   f = io.open(tmpfile)
   out = f:read '*a'
   f:close()
   os.remove(tmpfile)
   out, err = winapi.encode(-1,winapi.CP_UTF8,out)
   if err then return nil,err end
   return res,out
end
local function exec_cmd (cmd,arg)
    local res,err = winapi.execute(cmd..' "'..arg..'"')
    if res == 0 then return true
    else return nil,err
    end
end
function winapi.mkdir(dir) return exec_cmd('mkdir',dir) end
function winapi.rmdir(dir) return exec_cmd('rmdir',dir) end
function winapi.delete(file) return exec_cmd('del',file) end
function winapi.get_files(mask,subdirs,attrib)
    local flags = '/B '
    if subdirs then flags = flags..' /S' end
    if attrib then flags = flags..' /A:'..attrib end
    local ret, text = winapi.execute_unicode('dir '..flags..' "'..mask..'"')
    if ret ~= 0 then return nil,text end
    return text:gmatch('[^\r\n]+')
end
function winapi.get_dirs(mask,subdirs) return winapi.get_files(mask,subdirs,'D') end

files,err = winapi.get_files ('*.txt',false)
if not files then return print(err) end
for f in files do print(f) end








