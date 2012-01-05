require 'winapi'
local U = winapi.uexpand
local encode = winapi.encode
local UTF8 = winapi.CP_UTF8

winapi.set_encoding(UTF8)

local short = winapi.short_path
local name = short 'ελληνική.txt'
os.remove(name)
print(name)
local f,err = io.open(name,'w')
if not f then return print(err) end
f:write 'a new file\n'
f:close()


