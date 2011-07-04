require 'winapi'
t = os.clock()
winapi.sleep(200)
for i = 1,1e8 do  end
P = winapi.get_current_process()
print(P:get_working_size())
print(P:get_run_times())
print((os.clock() - t)*1000)
