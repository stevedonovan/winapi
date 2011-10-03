-- ldoc configuration file
file = "winapi.l.c"
output = "api"
title = "Winapi documentation"
project = "winapi"
readme = "readme.md"
--one = true
-- no_summary = true
examples = {'examples', exclude = {'examples/slow.lua'}}
description = [[
A minimal but useful binding to the Windows API.
]]

--manual_url 'file:///D:/dev/lua/projects/lua-5.1.4/doc/manual.html'

format = 'markdown'


