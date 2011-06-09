require 'winapi'
t = os.clock()
winapi.sleep(200)
for i = 1,1e8 do  end
P = winapi.current_process()
print(P:working_size())
print(P:run_times())
print((os.clock() - t)*1000)
