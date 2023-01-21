Embedded Xinuをはじめよう
==================================

ここでは :doc:`Embedded Xinu </Introduction>` のダウンロードと
コンパイルの方法について説明します。なお、LinuxやMac OS Xなどの
UNIX系OS、あるいは少なくとも `Cygwin <http://www.cygwin.com>`__
などのUNIX互換環境を使用していいることを前提にしてます。

.. contents::
   :local:

.. _downloading:

ソースコードのダウンロード
---------------------------

Embedded Xinuの安定版は、http://xinu-os.org/Downloads から
ダウンロードできます。

開発版（この記事を書いている時点では推奨）は
`git ソースコード管理システム <http://git-scm.com/>`__ を
を使ってリポジトリに保管されています。ダウンロードするには
`gitをインストール <http://git-scm.com/book/en/Getting-Started-Installing-Git>`__
して、以下のコマンドを実行してください。

.. code-block:: none

    $ git clone https://github.com/xinu-os/xinu

するとソースのコピーが得られたはずです。

.. code-block:: none

    $ cd xinu
    $ ls -F
    apps/    compile/  docs/     lib/     loader/   mem/ README.md  system/
    AUTHORS  device/   include/  LICENSE  mailbox/  network/  shell/ test/

Embedded XinuはBSDスタイルのライセンスでライセンスされて
いることに注意してください。詳細はソースディストリビューションに
ある著作権情報を参照してください。

プラットフォームの選択
-------------------------

:ref:`Embedded Xinuのサポートプラットフォーム一覧 <supported_platforms>`
を参照してください。

.. note::
   各サポートプラットフォームは ``compile/platforms/`` の
   サブディレクトリに該当します。

「本物の組み込みハードウェア」を持っておらず、単にノートPCや
デスクトップPCからEmbedded Xinuを試したいだけであれば
:doc:`mipsel-qemu <mips/Mipsel-qemu>` か
QEMU system emulator <http://qemu.org>`__ のいずれかのポートを
使うことができます。これらは `QEMUシステムエミュレータ <http://qemu.org>`__
で実行されます。

.. _cross_compiler:

クロスコンパイラの設定
---------------------------

Since most of Embedded Xinu's supported platforms do not share the
same processor architecture as an x86 workstation, building Embedded
Xinu typically requires an appropriate :wikipedia:`cross compiler` for
the C programming language.  The processor architecture of each
Embedded Xinu platform is listed under *Cross-target* in the
:ref:`list of supported platforms <supported_platforms>`; note that
this is the value to pass to ``--target`` when configuring binutils
and gcc.

Currently, only the `gcc compiler <http://gcc.gnu.org>`__ is
supported.  (clang does not yet work!)

選択肢 1: クロスコンパイラをリポジトリからインストール
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some Linux distributions already have popular cross compilers
available in their software repositories.  When available, this can be
used as a quick alternative to building from source.

選択肢 2: クロスコンパイラをソースからビルド
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This section documents how to build and install binutils and gcc from
source in a cross-compiler configuration.

ネイティブ開発環境
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before you can build anything from source, you first need appropriate
development tools for your native platform, such as **gcc** and
**make**.

- On Linux systems, these tools can be found in the software
  repositories under various names and groupings, depending on the
  Linux distribution.
- On Windows via `Cygwin <http://www.cygwin.com>`__, these tools can
  be found under the "devel" category when you run the setup program.
- On Mac OS X, these tools come with `Xcode
  <https://developer.apple.com/xcode/>`__.

binutils
^^^^^^^^

Before building the C compiler itself, the corresponding binary
utilities including the :wikipedia:`assembler <Assembler (computing)>`
and :wikipedia:`linker <Linker (computing)>` must be installed.

.. note::

   Good practice when building any software package is to use a normal
   user account, and only acquire root privileges with ``sudo`` for
   installation (step 6 below).

1. Download a recent release of `GNU binutils
   <https://www.gnu.org/software/binutils/>`__, for example:

   .. code-block:: none

      $ wget ftp://ftp.gnu.org/gnu/binutils/binutils-2.23.tar.gz

2. Untar the binutils source:

   .. code-block:: none

      $ tar xvf binutils-2.23.tar.gz

3. Create and enter a build directory:

   .. code-block:: none

      $ mkdir binutils-2.23-build
      $ cd binutils-2.23-build

4. Configure binutils for the appropriate target, for example:

   .. code-block:: none

      $ ../binutils-2.23/configure --prefix=/opt/mipsel-dev --target=mipsel \
               --disable-nls

   The argument given to ``--prefix`` is the location into which to
   install the binutils, and is of your choosing.  Typical locations
   would be a subdirectory of ``/opt`` or ``/usr/local``.  (Note that
   installing into these locations requires ``sudo`` privilege in
   step 6.  Normally, it is also possible to install software into a
   user's home directory, which does not require the ``sudo``
   privilege.)

   The argument given to ``--target`` is the target which the binutils
   will target, and must be set appropriately for the desired Embedded
   Xinu platform, as shown under *Cross-target* in the :ref:`list of
   supported platforms <supported_platforms>`.

   ``--disable-nls`` simply saves time and space by not supporting any
   human languages other than English.  You can skip this option if
   you want.

5. Build binutils:

   .. code-block:: none

      $ make

6. Install binutils:

   .. code-block:: none

      $ sudo make install

gcc
^^^

1. Download a recent release of the `GNU Compiler Collection
   <https://gcc.gnu.org>`__, for example:

   .. code-block:: none

      $ wget ftp://ftp.gnu.org/gnu/gcc/gcc-4.8.2/gcc-4.8.2.tar.bz2

2. Untar the gcc source:

   .. code-block:: none

      $ tar xvf gcc-4.8.2.tar.bz2

3. Create and enter a build directory:

   .. code-block:: none

      $ mkdir gcc-4.8.2-build
      $ cd gcc-4.8.2-build

4. Configure gcc for the appropriate target, for example:

   .. code-block:: none

      $ ../gcc-4.8.2/configure --prefix=/opt/mipsel-dev --target=mipsel \
                --enable-languages=c,c++ --without-headers --disable-nls

   ``--prefix`` and ``--target`` must be exactly the same as those
   chosen for the binutils installation.

   ``--enable-languages=c,c++`` ensures that only C and C++ compilers
   are built, not the compilers for other languages such as Ada and
   Fortran that are also supported by the GNU Compiler Collection.
   Note: Embedded Xinu does not actually contain C++ code, so if
   desired this could be stripped down to simply
   ``--enable-languages=c``.

   ``--without-headers`` is needed when there is no libc (standard C
   library) installed for the target platform, as is the case here.

   ``--disable-nls`` simply saves time and space by not supporting any
   human languages other than English.  You can skip this option if
   you want.

5. Build gcc:

   .. code-block:: none

      $ make all-gcc all-target-libgcc

   .. tip::

      gcc can take a while to build (upwards of half an hour).  You
      can add the argument ``-jN`` to **make**, where N is an integer,
      to run multiple compilation jobs in parallel.

6. Install gcc:

   .. code-block:: none

      $ sudo make install-gcc install-target-libgcc

クロスコンパイラのテスト
^^^^^^^^^^^^^^^^^^^^^^^^^^

First, for convenience you may wish to make the cross-utilities
available under their unqualified names by updating ``$PATH``, for
example:

.. code-block:: none

   export PATH="$PATH:/opt/mipsel-dev/bin"

The above should go in a shell startup file such as ``$HOME/.bashrc``.

Test the compiler by creating a file ``test.c``:

.. code-block:: c

    void f(void)
    {
    }

and compiling it with, for example::

    mipsel-gcc -c test.c

This should succeed and produce a file ``test.o`` without any error
messages.

.. _compiling:

Embedded Xinuのコンパイル
----------------------------

必要に応じてクロスコンパイラをビルドしたら、次にEmbedded Xinuを
コンパイルするには ``compile/`` にある ``Makefile`` を処理する
ために適当な ``PLATFORM`` を指定して **make** を実行する必要が
あります。たとえば、次のようにします。

.. code-block:: none

   $ make -C compile PLATFORM=wrt54gl

詳細については以下で説明します。

.. _makefile_variables:

Makefile変数
~~~~~~~~~~~~~~~~~~

ビルドをカスタマイズするために **make** コマンドラインでいくつかの
変数を定義することができます。

* ``PLATFORM`` にはカーネルをビルドするEmbedded Xinuプラット
  フォームである ``compile/platforms/`` 配下のディレクトリ名を
  指定します。

* ``COMPILER_ROOT`` にはターゲットプラットフォーム用のコードを
  コンパイル、アセンブル、リンクするために必要なコンパイラと
  binutilsの実行可能ファイルの場所を指定します。 ``COMPILER_ROOT``
  には実行可能ファイルの先頭にあるターゲットプレフィックスを含め
  なければなりません。ARMベースのプラットフォームを例にするとこれは
  ``/opt/arm-dev/bin/arm-none-eabi-`` です。実行可能ファイルが
  ``$PATH`` にある場合は単に ``arm-none-eabi-`` などのように指定
  することもできます。しかし、これ（または、ARM以外の
  ``PLATFORM`` 用の対応する接頭辞）はすでにデフォルトになっています。

* ``DETAIL`` は、Embedded Xinuでデバッグメッセージを有効にするために
  ``-DDETAIL`` として定義することができます。

* ``VERBOSE`` は、コンパイル、リンク、アセンブルなどの際に実行された
  実際のコマンドラインをビルドシステムに表示させるために任意の値に
  定義することができます。

以上の変数をオーバーライドするには、次の例のように **make** の引数と
して渡す必要があります。

    $ make PLATFORM=arm-rpi

.. _makefile_targets:

Makefileターゲット
~~~~~~~~~~~~~~~~~~~~~

次のMakefileターゲットを指定できます:

* **xinu.boot**
    Embedded Xinuをコンパイルします。デフォルトターゲットです。

* **debug**
    xinu.bootと同じですが、デバッグ情報を含めます。

* **docs**
    Embedded XinuのDoxygenドキュメントを生成します。Doxygenの
    インストールが必要です。注: ドキュメントから無関係な詳細を
    除去するためにドキュメントはプラットフォームでパラメタ化
    されています。そのため、生成されるドキュメントは ``PLATFORM``
    の現在の設定に依存します（ :ref:`makefile_variables` を参照）。

* **clean**
    すべてのオブジェクトファイルを削除します。

* **docsclean**
    ``make docs`` により生成されたドキュメントを削除します。

* **realclean**
    生成されたあらゆる種類のファイルをすべて削除します。

以上で重要なターゲットはカバーしていますが、その他に利用可能な
ターゲットについては ``compile/Makefile`` を参照してください。

.. note::
    古いバージョンのEmbedded Xinuにはヘッダーの依存情報を生成する
    ための ``make depend`` ターゲットがありました。現在ではこの
    情報は自動的に生成されるため、このターゲットは削除されました。
    つまり、ヘッダーを変更した場合、適切なソースファイルが自動的に
    再コンパイルされるようになりました。

次のステップ
--------------

通常、 :ref:`Embedded Xinuをコンパイル <compiling>` すると
カーネルバイナリを含むファイル ``xinu.boot`` が生成されます。
実際にこのファイルの実行する方法はほとんどがプラットフォームに
依存します。ほんの数例ですが、以下のようなものがあります。

- Raspberry Pi: :ref:`raspberry_pi_booting` と
  :ref:`xinupi_getting_started` を参照してください。
- Mipsel-QEMU: :doc:`/mips/Mipsel-qemu` を参照してください。
- ARM-QEMU: :doc:`/arm/ARM-qemu` を参照してください。

次に読むべきドキュメント:

- :doc:`features/index`
- :doc:`teaching/index`

その他の情報源
---------------
- `GCC Cross-Compiler (OSDev Wiki) <http://wiki.osdev.org/GCC_Cross-Compiler>`__
