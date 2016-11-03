(Lib)underline is a small <a href="https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html">libreadline</a> compatible commandline C library. Command line interface (CLI) may well be the <a href="http://cristal.inria.fr/~weis/info/commandline.html">first</a> and arguably one of the most intuitive human-computer interfaces.

I wrote underline when I needed a CLI for an embedded system without operating system and a very limited (and horribly outdated) libc support. My original thought was to port libreadline, the de-facto standard. But in readline's own words:
"It's too big and too slow" (<a href="https://linux.die.net/man/3/readline" target="_blank">man 3 readline</a> - Bugs section).

Underline implements a compatible (although partial) commandline library. The entire implementation is less than 500 lines of code and has some nice features in it:
<ul>
	<li>Standard synchronic <em>readline</em> API for basic usage</li>
	<li>Fully asynchronous API</li>
	<li>History support (although quite limited)</li>
	<li>Hotkeys support (for home/end etc.)</li>
	<li>Basic VT-100 compatibility. Support standard termios library or custom VT implementation.</li>
	<li>Fits embeeded systems - No memory allocations, small code size</li>
</ul>
The asynchronous API is suitable for systems requiring high performance. It was tested on Posix AIO, Arduino and RTOS like systems.

Sources:
<h2>TODO</h2>
The implementation is, well, minimal. There's lots to do, especially if you have special needs.

Custom hotkey mapping - At the moment all hotkeys are hardcoded. Should be user override-able.

Command completion - Hitting TAB should call a user defined callback with the partial current word (and the full line).

Dynamic history buffer - The history buffer is static and not optimized. You could change the defines to get more history or implement a custom memory allocator or an arena if you need dynamic history size.

Dynamic prompt - Prompt can be used for fun stuff like showing the current path, Git branch or number of unread emails. Functionality to update the prompt with some helpers is a nice feature.

Terminal width/multiline support - Receive resize events from terminal and support multiline prompt (like bash's '\' or iPython's Shift+Enter)

Thread safety - completely not written to be thread safe. The synchrous API has a static context and none of the functions was tested to be reentrant. If you want, make the context struct <a href="https://gcc.gnu.org/onlinedocs/gcc-3.3.1/gcc/Thread-Local.html">TLS</a> or protect it and check the functions.

More modular code - History should probably be a module
