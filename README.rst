Put the files or symlinks to them in ``source/tools/ManualExamples``.

Then compile with (replace ``file`` for either ``arqsimucache`` or
``arqsimujumps``)::

    make dir obj-intel64/file.so

And run with:: 

    ../../../pin -injection child -t obj-intel64/file.so -- /bin/ls

To debug, follow instructions in
http://www.pintool.org/docs/45467/Pin/html/. You will probably use
something like the following, to pause while you attach to the process using
gdb::

    ../../../pin -pause_tool 10 -injection child -t obj-intel64/file.so -- /bin/ls

