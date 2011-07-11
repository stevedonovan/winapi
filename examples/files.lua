-- iterating over all files matching some pattern.
-- (this handles Unicode file names correctly)
require 'winapi'
winapi.set_encoding(winapi.CP_UTF8)

files,err = winapi.files ('*.txt',false)
if not files then return print(err) end

for f in files do
  print(f)
end

