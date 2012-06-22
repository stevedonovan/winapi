require 'winapi'
k,err = winapi.open_reg_key [[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\DateTime\Servers]]
if not k then return print('bad key',err) end

print(k:get_value("1"))
k:close()

k,err = winapi.open_reg_key [[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]]

t = k:get_keys()
--for _,k in ipairs(t) do print(_,k) end
k:close()

k,err = winapi.open_reg_key ([[HKEY_CURRENT_USER\Environment]],true)
path = k:get_value("PATH")
print(path)
if #arg > 0 then
    --print(k:set_value("PATH",path..';'..arg[1]))
    --print(k:get_value("PATH"))
    k:set_value(arg[1],arg[2])
    print(k:get_value(arg[1]))
end
k:close()



