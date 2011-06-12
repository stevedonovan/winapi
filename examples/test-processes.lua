require 'winapi'
pids = winapi.get_processes()

for _,pid in ipairs(pids) do
   local P = winapi.process(pid)
   local name = P:get_process_name(true)
   if name then print(pid,name) end
   P:close()
end
