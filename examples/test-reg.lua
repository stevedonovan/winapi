require 'winapi'
k,err = winapi.open_reg_key [[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\DateTime\Servers]]
if not k then return print('bad key',err) end
print(k:get_value(""))
k:close()

k,err = winapi.open_reg_key [[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]]

t = k:get_keys()
for _,k in ipairs(t) do print(_,k) end


--"\\Software\\Policies\\Microsoft\\Windows\\WindowsUpdate")
-- [[SYSTEM\CurrentControlSet\Services\Tcpip\Parameters]])
