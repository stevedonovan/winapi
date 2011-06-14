require 'winapi'
p = winapi.current_process()
print(os.date('%c',os.time(p:start_time())))

