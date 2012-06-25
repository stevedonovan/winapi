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
print(k:get_value("TEMP"))
if #arg > 0 then
    local type = winapi.REG_SZ
    if arg[3] then
	type = winapi[arg[3]]
    end
    k:set_value(arg[1],arg[2],type)
    print(k:get_value(arg[1],type))
end
k:close()



