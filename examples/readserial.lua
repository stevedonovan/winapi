require 'winapi'
local f,e = winapi.open_serial 'COM4 baud=19'
if not f then return print('error',e) end
local sub = f:read()
local line = {}
local append = table.insert
while sub ~= '+' do
  f:write(sub)
  append(line,sub)
  if sub == '\r' then
	f:write '\n'
	print('gotcha',table.concat(line))
	line = {}
  end
  --print(sub,sub:byte(1))
  sub = f:read()
end
f:close()
