シェル
========

**XINUシェル** (**xsh**)は人がオペレーティングシステムと対話する
ための簡単なコマンドラインインターフェイスとして機能するサブ
システムです。 :source:`shell/` に実装されています。

動作の仕組み
----------------

シェルの起動
~~~~~~~~~~~~~~~~

.. note:: このセクションではプログラムでシェルを起動する方法について
          説明します。デフォルトではこれはすでに :source:`system/main.c`
          により実行されています。

XINUシェルのインスタンスは :source:`shell() <shell/shell.c>`
プロシージャを実行する :source:`スレッドを作成する <system/create.c>`
ことで開始できます。このプロシージャは次のように宣言されています。

.. code:: c

  thread shell(int indescrp, int outdescrp, int errdescrp);

シェルは ``indescrp`` で指定されたキャラクタデバイスからコマンドを
読み込んで実行します。実行されたシェルコマンドによって
:source:`stdout <include/stdio.h>` に書き込まれた出力は ``outdescrp``
で指定されたデバイスに、 :source:`stderr <include/stdio.h>` に
書き込まれた出力は ``errdescrp`` で指定されたデバイスに送られます。

:source:`system/main.c` で見られるように、シェルを起動する典型的な
例は次のとおりです。

.. code:: c

    ready(create
              ((void *)shell, INITSTK, INITPRIO, "SHELL0", 3,
                         CONSOLE, CONSOLE, CONSOLE), RESCHED_NO);

ここではすべての入出力に ``CONSOLE`` デバイスを使用していますが、
通常、これは最初のシリアルポート、すなわちUARTをラップする
:doc:`TTYデバイス <TTY-Driver>` としてセットアップされます。

.. code:: c

    open(CONSOLE, SERIAL0);

キーボードやフレームバッファ、または、追加のシリアルポートなどの
入出力デバイスが利用可能な場合は、さらにシェルスレッドを起動する
ことができます。

コマンドの読み込みと実行
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ユーザがシェルでコマンドを入力すると :source:`lexan() <shell/lexan.c>`
関数が入力文字列をトークンに分割します。コマンド名、引数、引用符で
囲まれた文字列、バックグラウンドトークン、 リダイレクトトークンの
すべてが :source:`lexan() <shell/lexan.c>` により認識され、分割
されます。

コマンドがパースされると、シェルはトークンを使用して与えられた
コマンドを適切に実行します。シェルはまずバックグラウンド実行を
指定するアンパサンド ('&') をチェックします。これは最後のトークンと
してしか現れないはずです。シェルはリダイレクトを処理するように
設計されていますが、XINUのファイルシステムは開発中であるため、
現在は処理されません。

次に、コマンドが :source:`shell/shell.c` の冒頭で定義されている
コマンドテーブルから検索されます。コマンドテーブルの各エントリは
``{"command_name", TRUE / FALSE, xsh_function}`` :
すなわち、コマンド名、ビルトイン関数の是非（すなわち、バック
グラウンドで実行可能か）、コマンドを実行する関数からなる
フォーマットで記述されています。

ビルトインコマンドはそのコマンドを実装している関数を呼び出すことで
実行されます。他のすべてのコマンドは新しいプロセスを作成することに
より実行されます。ユーザが入力にバックグラウンド化フラグを含めなかった
場合、シェルはコマンドのプロセスが完了するまで待ってから、さらに
入力を求めます。

コマンド一覧
----------------

XINUのビルドで実際に利用できるシェルコマンドは、プラットフォームや
有効化された機能によって異なりますが、重要なコマンドを以下に
リストアップします。

=============   ===========
コマンド        説明
=============   ===========
**clear**       シェルの出力をクリアします
**exit**        シェルを終了します
**help**        サポートコマンド、または特定のコマンドヘルプを表示します
**kill**        指定のスレッドをkillします
**memstat**     現在のメモリ使用状況を表示し、フリーリストを出力します
**memdump**     メモリ領域をダンプします
**ps**          実行中のプロセス一覧を表示します
**reset**       システムをソフトリセットします
**sleep**       指定した時間だけ実行中のスレッドをsleepさせます
**test**        デフォルトでは何もしませんが、開発者はここに一時的にコードを追加できます
**testsuite**   システムが正しく機能しているかを調べる一連のテストを実行します
**uartstat**    UARTに関する情報を表示します
=============   ===========

コマンドの完全なリストはシェル上で ``help``  コマンドを実行する
ことにより得られます。特定のコマンドのヘルプは `COMMAND --help``
または ``help COMMAND`` で得られます。

コマンドの追加
---------------

The shell is designed to be expandable, allowing users to add their
own commands. The code that runs the shell (:source:`shell/shell.c`)
and the command parser (:source:`shell/lexan.c`) do not need to change
when a new command is added. The majority of the work goes into
writing the actual command. After the command is written, three items
must be added to the system:

-  the function prototype needs to be added to the header file
   (:source:`include/shell.h`),
-  the command table (:source:`shell/shell.c`) must be updated, and
-  the make file (:source:`shell/Makerules`) must build the file
   containing the function.

Writing the function
~~~~~~~~~~~~~~~~~~~~

The command should be given its own C source file in the :source:`shell/`
directory, following the naming convention ``xsh_command.c``. All
command files should include ``kernel.h`` and ``shell.h``, along with
any other headers necessary for the command. Function names for commands
follow the same naming convention as the source file: ``xsh_command``.
The method signature for a command is:

.. code:: c

  shellcmd xsh_command(int nargs, char *args[])

Within the command, arguments are accessed via the ``args`` array. The
command name is located in ``arg[0]``. Subsequent arguments, up to
``nargs`` are accessed via ``arg[n]``. Error checking of arguments is
the responsibility of the command function. It is good practice to check
for the correct number of arguments; remember the command name is
counted in ``nargs``, so a command without any arguments should have
``nargs == 1``. Although not required, command functions should also
allow for an argument of ``--help`` as ``arg[1]``. This argument should
cause the command to print out usage information. When a user types
``help COMMAND`` in the shell, the ``COMMAND`` is called with the
``--help`` argument.

Additional code within the command function depends on what the command
does. After the command is completed it should return ``OK``.

Add to command table
~~~~~~~~~~~~~~~~~~~~

After the command function is written, the command needs to be added to
the command table so the shell is aware of the command. The command
table is an array of ``centry`` (command entry) structures defined in
``shell/shell.c``. Each entry in the command table follows the format of
command name, is the command built-in (ie can the command run in the
background), and the function that executes the command:
``{"command_name", TRUE / FALSE, xsh_function},``.

Add to header and makefile
~~~~~~~~~~~~~~~~~~~~~~~~~~

To complete the process, add the function prototype to the shell header
file ``include/shell.h``:

.. code:: c

    shellcmd xsh_command(int, char *[]);

Lastly, add the command function source file to the makefile
(``shell/Makerules``) under the ``C_FILES`` group to ensure the command
is compiled into the XINU boot image.

Example
~~~~~~~

We will run through a brief implementation of adding an echo command to
the system.

Write the function
^^^^^^^^^^^^^^^^^^

Begin by creating the source file in ``shell/xsh_echo.c``. Since all
commands take the same arguments (as passed by the shell), we get:

.. code:: c

    #include <kernel.h>
    #include <stdio.h>
    #include <string.h>

    /**
     * Shell command echos input text to standard out.
     * @param stdin descriptor of input device
     * @param stdout descriptor of output device
     * @param stderr descriptor of error device
     * @param args array of arguments
     * @return OK for success, SYSERR for syntax error
     */
    shellcmd xsh_echo(int nargs, char *args[])
    {
        int i;  /* counter for looping through arguments */

        /* Output help, if '--help' argument was supplied */
        if (nargs == 2 && strcmp(args[1], "--help") == 0)
        {
            fprintf(stdout, "Usage: clear\n");
            fprintf(stdout, "Clears the terminal.\n");
            fprintf(stdout, "\t--help\t display this help and exit\n");
            return SYSERR;
        }

        /* loop through the arguments printing each as it is displayed */
        for ( i = 1; i < nargs; i++ )
        {
            fprintf(stdout, "%s ", args[i]);
        }

        /* Just so the next prompt doesn't run on to this line */
        fprintf(stdout, "\n");

        /* there were no errors so, return OK */
        return OK;
    }

Add the function to the command table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While we are in the :source:`shell/` directory, we'll modify the command table
found at the top of :source:`shell/shell.c`.  Since we are adding the echo
command, we'll most likely want the user input at the shell to be
"``echo``," this is not a builtin function (FALSE), and the function
that supports this is xsh\_echo. Giving us the entry:

.. code:: c

    { "echo", FALSE, xsh_echo }

Add the function prototype to the include file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Next we must add the prototype of the function to the shell include
file in :source:`include/shell.h`. This is simply done by adding the
line:

.. code:: c

    shellcmd xsh_echo(int, char *[]);

Add the file to the Makefile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Finally (and most importantly) we add the function to the Makefile to
make sure that it is built by the compiler. We do this by finding the
line beginning with "``C_FILES =``\ " in ``shell/Makerules`` and adding
xsh\_echo.c to the end of it.

Compile and run, and you should now have a working implementation of the
``echo`` command on your XINU system!
