Put the files or symlinks to them in ``source/tools/ManualExamples``.

Open the ``makefile`` in that directory, and add
``arqsimucache arqsimujumps arqsimucpu`` to the ``TOOL_ROOTS``
variable.

Then compile with (replace ``file`` for either ``arqsimucache``,
``arqsimujumps`` or ``arqsimucpu``)::

    make dir obj-intel64/file.so

And run with (replace ``/bin/ls`` with any executable):: 

    ../../../pin -injection child -t obj-intel64/file.so -- /bin/ls

Every tool creates a file called ``file.out`` (for example,
``arqsimucache`` creates ``arqsimucache.out``) in the working
directory.

In case you need to debug, follow instructions in
http://www.pintool.org/docs/45467/Pin/html/. You will probably use
something like the following, to pause while you attach to the process using
gdb::

    ../../../pin -pause_tool 10 -injection child -t obj-intel64/file.so -- /bin/ls
