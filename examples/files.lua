require 'winapi'
winapi.set_encoding(winapi.CP_UTF8)
files,err = winapi.get_files ('*.txt',false)
if not files then return print(err) end
for f in files do print(f) end
