REM convert the markdown to HTML
lua markdown.lua readme.md -s doc.css -l 
copy readme.html docs\index.html
copy doc.css docs\default.css
del readme.html

