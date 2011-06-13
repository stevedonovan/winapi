require 'winapi'

drives = winapi.get_logical_drives()
for _,drive in ipairs(drives) do
    local free,avail = winapi.get_disk_free_space(drive)
    if not free then -- call failed, avail is error
        free = '('..avail..')'
    else
        free = math.ceil(free/1024) -- get Mb
    end
    local rname = ''
    local dtype = winapi.get_drive_type(drive)
    if dtype == 'remote' then
        rname = winapi.get_disk_network_name(drive:gsub('\\$',''))
    end
    print(drive,dtype,free,rname)
end
