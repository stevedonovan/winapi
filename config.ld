-- ldoc configuration file
file = "winapi.l.c"
output = "api"
title = "Winapi documentation"
project = "winapi"
readme = "readme.md"
-- no_summary = true
examples = {'examples', exclude = {'examples/slow.lua'}}
description = [[
A minimal but useful binding to the Windows API.
]]

format = 'markdown'


