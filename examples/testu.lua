require 'winapi'
local U = winapi.utf8_expand
local UTF8 = winapi.CP_UTF8

winapi.set_encoding(UTF8)

txt = U '%u03BB + %u03BC + C'
print(txt)

print(U '%u03BD')
