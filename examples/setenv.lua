-- You will only get nice output from this script (like other unicode examples)
-- if executed in a properly multilingual environment like SciTE.
-- To get UTF-8 support in SciTE, edit your global properties like so:
-- # Internationalisation
-- # Japanese input code page 932 and ShiftJIS character set 128
-- #code.page=932
-- #character.set=128
-- # Unicode
-- code.page=65001  # uncomment out this line
-- #code.page=0  # and comment out this line
--
-- And restart SciTE.

require 'winapi'

winapi.setenv('greek','ελληνική')

print(os.getenv 'greek') -- this will still be nil

-- but child processes can see this variable ...
os.execute [[lua -e "print(os.getenv('greek'))"]]
