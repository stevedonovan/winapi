## winapi A useful Windows API subset for Lua

This module provides some basic tools for working with Windows systems, finding out system resources, and gives you more control over process creation.  In this introduction any plain reference is in the `winapi` table, so that `find_window` means `winapi.find_window`.

### Creating and working with Processes

An  irritating fact is that Lua GUI applications (such as IUP or wxLua) cannot use `os.execute` without the infamous 'flashing black box' of console creation. And `io.popen` may in fact not work at all.

`execute` provides a _quiet_ method to call a shell command.  It returns the result code (like `os.execute`) but also any text generated from the command. So for many common applications it will do as a `io.popen` replacement as well. This function is blocking, but `winapi` provides more general ways of launching processes in the background and even capturing their output asynchronously. This will be discussed later with `spawn`.

Apart from `execute`, `shell_exec` is the Swiss-Army-Knife of Windows process creation. The first parameter is the 'action' or 'verb' to apply to the path; common actions are 'open', 'edit' and 'print'. Notice that these are the actions defined in Explorer (hence the word 'shell'). So to open a document in Word (or whatever application is registered for this extension):

    winapi.shell_exec('open','myold.doc')

Or an explorer window for a directory:

    winapi.shell_exec('open','\\users\\steve\\lua')

Note that this function launches the process and does not block. The path may be an explicit program to use, and then we can also specify the command-line parameters:

    winapi.shell_exec(nil,'scite','wina.lua')

The fourth parameter is the working directory for the process, and the fifth indicates how the program's window is to be opened. For instance, you can open a file in Notepad already minimized:

    winapi.shell_exec(nil,'notepad','wina.lua',nil,winapi.SW_MINIMIZE)

For fine control over console programs, use `winapi.spawn` - you pass it the command-line, and receive two values; a process object and a file object. You monitor the process with the first, and can read from or write to the second.

    > proc,file = winapi.spawn 'cmd /c dir /b'
    > = file:read()
    bonzo.lc
    cexport.lua
    class1.c
    ...
    > = proc:wait()
    userdata: 0000000000539608      OK
    > = proc:exit_code()
    0

If the command is invalid, then you will get an error message instead:

    > = winapi.spawn 'frodo'
    nil     The system cannot find the file specified.

This is what `execute` does under the hood, but doing it explicitly gives you more control.  For instance, the `wait` method of the process object can take an optional time-out parameter; if you wait too long for the process, it will return the process object and the string 'TIMEOUT'.

    local _,status = proc:wait(500)
    if status == 'TIMEOUT' then
      proc:kill()
    end

The file object is unfortunately not a Lua file object, since it is not possible to _portably_ re-use the existing Lua implementation without copying large chunks of `liolib.c` into this library. So `read` grabs what's available, unbuffered. But I feel that it's easy enough for Lua code to parse the result into separate lines, if needed.

Having a `write` method means that, yes, you can capture an interactive process, send it commands and read the result. The caveat is that this process must not buffer standard output. For instance, launch interactive Lua with a command-line like this:

    > proc,file = winapi.spawn [[lua -e "io.stdout:setvbuf('no')" -i]]
    > = file:read()  -- always read the program banner first!
    Lua 5.1.4  Copyright (C) 1994-2008 Lua.org, PUC-Rio
    >
    > = file:write 'print "hello"\n'
    14
    > = file:read()
    hello
    >
    > proc:kill()

(We also found it necessary in the [Lua for Windows]() project to switch off buffering for using Lua in SciTE)

Note that reading the result also returns the prompt '>', which isn't so obvious if we're running Lua from within Lua itself. It's clearer when using Python:

    > proc,file = winapi.spawn [[python -i]]
    > = file:read()
    Python 2.6.2c1 (r262c1:71369, Apr  7 2009, 18:44:00) [MSC v.1500 32 bit (Intel)]
     on win32
    Type "help", "copyright", "credits" or "license" for more information.
    >>>
    > file:write '40+2\n'
    > = file:read()
    42
    >>>

This kind of interactive process capture is fine for a console application, but `read` is blocking and will freeze any GUI program. For this, you use `read_async` which returns the result through a callback.

    > file:write '40+2\n'
    > file:read_async(function(s) print('++',s) end)
    > ++    42
    >>>

This can work nicely with Lua coroutines, allowing us to write pseudo-blocking code for interacting with processes.

The process object can provide more useful information:

    > = proc:working_size()
    200     1380
    > = proc:run_times()
    0       31

`working_size` gives you a lower and an upper bound on the process memory in kB; `run_times` gives you the time (in milliseconds) spent in the user process and in the kernel. So the time to calculate `40+2` twice is too fast to even register, and it has only spent 31 msec in the system.

It is possible to wait on more than one process at a time. Consider this simple time-wasting script:

    for i = 1,1e8 do end

It takes me 0.743 seconds to do this. But running two such scripts in parallel is about the same speed (0.776):

    require 'winapi'
    local t = os.clock()
    local P = {}
    P[1] = winapi.spawn 'lua slow.lua'
    P[2] = winapi.spawn 'lua slow.lua'
    winapi.wait_for_processes(P,true)
    print(os.clock() - t)

So my i3 is effectively a two-processor machine; four such processes take 1.325 seconds, just under twice as long. The second parameter means 'wait for all'; like the `wait` method, it has an optional timeout parameter.

### Working with Windows

The windows object provides methods for querying window properties. For instance, the desktop window fills the whole screen, so to find out the screen dimensions is straightforward:

    > = winapi.desktop_window():get_bounds()
    1600    900

Finding other windows is best done by iterating over all top-level windows and checking them for some desired property; (`find_window` is provided for completeness, but you really have to provide the exact window caption for the second parameter.)

`find_all_windows` returns all windows matching some function. For convenience, two useful matchers are provided, `match_name` and `match_class`. Once you have a group of related windows, you can do fun things like tile them:

    > t = winapi.find_all_windows(winapi.match_name '- SciTE')
    > = #t
    2
    > winapi.tile_windows(winapi.desktop_window(),false,t)

This call needs the parent window (we just use the desktop), whether to tile horizontally, and a table of window objects.  There is an optional fourth parameter, which is the bounds to use for the tiling, specified like so `{left=0,top=0,right=600,bottom=900}`.

With tiling and the ability to hide windows with `w:show(winapi.SW_HIDE)` it is entirely possible to write a little 'virtual desktop' application.

`find_window_ex` also uses a matcher function; `find_window_match` is a shortcut for the operation of finding a window by its caption.

Every window has an associated text value. For top-level windows, this is the window caption:

    > = winapi.foreground_window()
    Command Prompt - lua -lwinapi

So the equivalent of the old DOS command `title` would here be:

    winapi.foreground_window():set_text 'My new title'

Any top-level window will contain child windows. For example, Notepad has a simple structure revealed by `enum_children`:

    > w = winapi.find_window_match 'Notepad'
    > = w
    Untitled - Notepad
    > t = {}
    > w:enum_children(function(w) table.insert(t,w) end)
    > = #t
    2
    > = t[1]:get_class_name()
    Edit
    > = t[2]:get_class_name()
    msctls_statusbar32

Windows controls like the 'Edit' control interact with the unverse via messages.
`EM_GETLINECOUNT` will tell the control to return the number of lines. Looking up the numerical value of this message, it's easy to query Notepad's edit control:

    > = t[1]:send_message(186,0,0)
    6

An entertaining way to automate some programs is to send virtual keystrokes to them. The function `send_input` sends characters to the current foreground window:

    > winapi.send_input '= 20 + 10\n'
    > = 20 + 10
    30

After launching a window, you can make it the foreground window and send it text:

    winapi.shell_exec(nil,'notepad')
    winapi.sleep(100)
    notew = winapi.find_window_match 'Untitled'
    notew:set_foreground()
    winapi.send_input 'Hello World!'

The little sleep is important: it gives the other process a chance to get going, and to create a new window which we can promote.

### File and Directory Operations

There are functions for querying the filesystem: `get_logical_drives()` returns all available drives (in 'D:\\' format) and `get_drive_type()` will tell you whether these drives are fixed, remote, removable, etc. `get_disk_free_space()` will return the space used and the space available in kB as two results.

    require 'winapi'

    drives = winapi.get_logical_drives()
    for _,drive in ipairs(drives) do
        local free,avail = winapi.get_disk_free_space(drive)
        if not free then -- call failed, avail is error
            free = '('..avail..')'
        else
            free = math.ceil(free/1024) -- get Mb
        end
        print(drive,winapi.get_drive_type(drive),free)
    end

This script gives the following output on my machine:

    C:\	fixed	218967
    F:\	fixed	1517
    G:\	cdrom	(The device is not ready.)
    Q:\	fixed	(Access is denied.)

A useful operation is watching directories for changes. You specify the directory, the kind of change to monitor and whether subdirectories should be checked. You also provide a function that will be called when something changes.

    winapi.watch_for_file_changes(mydir,winapi.FILE_NOTIFY_CHANGE_LAST_WRITE,FALSE,
        function(what,who)
            -- 'what' will be winapi.FILE_ACTION_MODIFIED
            -- 'who' will be the name of the file that changed
            print(what,who)
        end
    )

Using a callback means that you can watch multiple directories and still respond to timers, etc.

 Finally, `copy_file` and 'move_file` are indispensible operations which are surprisingly tricky to write correctly in pure Lua. For general filesystem operations like finding the contents of folders, I suggest a more portable library like [LuaFileSystem](). However, you can get pretty far with a well-behaved way to call system commands:

    local status,output = winapi.execute('dir /B')
    local files = {}
    for f in output:gmatch '[^\r\n]+' do
        table.insert(files,f)
    end

### Output and Timers

GUI applications do not have a console so `print` does not work. `show_message` will put up a message box to bother users, and `output_debug_string` will write text quietly to the debug stream. A utility such as [DebugView]() can be used to view this output, which shows it with a timestamp.

It is straightforward to create a timer. You could of course use `sleep` but then your application will do nothing but sleep most of the time. This callback-driven timer can run in the background:

    winapi.timer(500,function()
        text:append 'gotcha'
    end)

Such callbacks can be made GUI-safe by first calling `use_gui` which ensures that any callback is called in the main GUI thread.

The basic rule for callbacks enforced by `winapi` is that only one may be active at a time; otherwise we would risk re-entering Lua using the same state. So be quick when responding to callbacks, since they effectively block Lua. For a console application, the best bet (after setting some timers and so forth) is just to sleep indefinitely:

    winapi.sleep(-1)

### Reading from the Registry

### Pipe Server

Interprocess communication (IPC) is one of those tangled, operating-system-dependent things that can be terribly useful. On Unix, _named pipes_ are special files which can be used for two processes to easily exchange information. One process opens the pipe for reading, and the other process opens it for writing; the first process will start reading, and this will block until the other process writes to the pipe. Since pipes are a regular part of the filesystem, two Lua scripts can use regular I/O to complete this transaction.

Life is more complicated on Windows (as usual) but with a little bit of help from the API you can get the equivalent mechanism from Windows named pipes. They do work a little differently; they are more like Unix domain sockets; a server waits for a client to connect ('accept') and then produces a handle for the new client to use; it then goes back to waiting for connections.

    require 'winapi'

    winapi.server(function(file)
      file:read_async(function(s) print('['..s..']') end)
    end)

    winapi.sleep(-1)

Like timers and file notifications, this server runs in its own thread so we have to put the main thread to sleep.  This function is passed a callback and a pipe name; pipe names must look like '\\\\.\\pipe\\NAME' and the default name is '\\\\.\\pipe\\luawinapi'. The callback receives a file object - in this case we use `read_async` to play nice with other Lua threads. Multiple clients can have open connections in this way, up to the number of available pipes.

The client can connect in a very straightforward way:

    > f = io.open('\\\\.\\pipe\\luawinapi','w')
    > f:write 'hello server!\n'
    > f:flush()
    > f:close()

and our server will say:

    [hello server!
    ]
    []

(Note that `read` receives an _empty string_ when the handle is closed.)

However, we can't push 'standard' I/O very far here. So there is also a corresponding `open_pipe` which returns a file object, both readable and writeable. It's probably best to think of it as a kind of socket; each call to `read` and `write` are regarded as receive/send events.

The server can do something to the received string and pass it back:

    winapi.server(function(file)
      file:read_async(function(s)
       if s == '' then
         print 'client disconnected'
       else
        file:write (s:upper())
       end
     end)
    end)

On the client side:

    f = winapi.open_pipe()
    f:write 'hello\n'
    print(f:read()) -- HELLO
    f:write 'dog\n'
    print(f:read()) -- DOG
    f:close()

Another simularity with sockets is that you can connect to remote pipes (see [pipe names](http://msdn.microsoft.com/en-us/library/aa365783(v=vs.85).aspx))



