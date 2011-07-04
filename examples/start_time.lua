require 'winapi'
p = winapi.get_current_process()
print(os.date('%c',os.time(p:get_start_time())))

