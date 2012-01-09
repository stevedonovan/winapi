require 'winapi'

f = winapi.get_console()
f:read_async(function(line)
  f:write(line)
  if line:match '^quit' then
    os.exit()
  end
end)

winapi.sleep(-1)
