Put the file or a symlink to it in ``source/tools/ManualExamples``.

Then compile with::

    make dir obj-intel64/arqsimucache.so

And run with:: 

    ../../../pin -injection child -t obj-intel64/arqsimucache.so -- /bin/ls

To debug, follow instructions in
`http://www.pintool.org/docs/45467/Pin/html/`_. You will probably use
something like the following, to pause while you attach to the process using
gdb::

    ../../../pin -pause_tool 10 -injection child -t obj-intel64/arqsimucache.so -- /bin/ls

