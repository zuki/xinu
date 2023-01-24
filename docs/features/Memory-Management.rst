メモリ管理
=================

メモリ管理はすべてのオペレーティングシステムにおいて重要な要素です。
そのため、XINUでは使用するハードウェアの機能を利用して、わかりやすい
メモリ管理システムを構築しています。

メモリアロケータ
-----------------

XINUでは2つのメモリアロケータを持っており、これらは連携してカーネルと
ユーザソフトウェアに動的にメモリを提供します。第1のアロケータは
カーネルアロケータであり、カーネルが必要とする小さなメモリチャンクを
グローバルメモリーヒープから割り当てます。第2のアロケータはユーザ
アロケータであり、ユーザプロセスが必要とするメモリをスレッドごとの
メモリヒープから割り当てます。

カーネルアロケータ
~~~~~~~~~~~~~~~~~~~

システムで最も基本的なメモリアロケータは ``memget`` 関数と
``memfree`` 関数を使用するカーネルアロケータです。これはグローバル変数
``memlist`` を使用するグローバルカーネルヒープ上で動作します。
このアロケータはカーネル開発者がメモリブロックの会計情報を追跡
するものと信用しています。このため、かなりわかりやすいAPIに なって
います。

.. code:: c

    void *memptr = memget(nbytes);
    memfree(memptr, nbytes);

このAPIからわかるように、アロケーション関数は要求するバイト数である1つの
パラメータ (``nbytes``) を取ります。でアロケーション関数は2つのパラメタ
(``memptr`` と ``nbytes``) を取ります。 ``memptr`` は ``memget`` 関数
関数で割り当てたメモリのアドレスであり、 ``nbytes`` は割り当て関数の
呼び出しで要求したバイト数です。

ユーザアロケータ
~~~~~~~~~~~~~~~~~~

カーネルアロケータとは異なり、ユーザアロケータはプログラマが要求した
メモリ量を覚えていることを信用せず、代わりに割り当てられたメモリの
直前に会計情報を格納します。プログラマにとってユーザメモリのAPIは
単純なものです。

.. code:: c

    void *memptr = malloc(nbytes);
    free(memptr);

このアロケータはスレッドごとの空きメモリリストで動作します。
これによりメモリは呼び出し元のスレッドが所有することになり、他の
スレッドがこのメモリにアクセスするのを防ぐことができます。これが
メモリ保護の基礎となっています。

アロケータにメモリ要求が来ると、すでにスレッドに割り当てられている
空きメモリで要求を満たそうとします。それが失敗するとアロケータは
リージョンアロケータ（後述）からメモリを獲得しようとします。リージョン
アロケータはページ単位で動作するので、余ったメモリは将来の要求のために
スレッドの空きメモリリストに挿入されます。メモリブロックが解放されると
そのメモリはスレッドの空きメモリリストに戻されます。

メモリがスレッドの保護領域から削除され、リージョンアロケータが利用
できるようになるのはスレッドがkillされた後です。

リージョンアロケータ
^^^^^^^^^^^^^^^^^^^^^^^^^

リージョンアロケータはユーザアロケータの下で働き、ブートプロセス中に
初期化されます。システムブート時にXINUは ``xinu.conf`` で定義されている
``UHEAP_SIZE`` を使用してユーザヒープ用のメモリを割り当てます。この
メモリはカーネルの ``memget()`` 関数により割り当てられ
``memRegionInit()`` 関数に渡されます。リージョンアロケータは初期化
されるとリージョンアロケータへの唯一のユーザレベルのインタフェースは
``malloc`` 関数と ``free`` 関数の背後に隠蔽されます。

メモリ保護
-----------------

.. note::

   このセクションはMIPSプラットフォームにしか関係しません。

Since XINU has limited resources to work with it does not provide a
virtual memory system. It does take advantage of separate address
spaces for each user thread running in the system, which provides
simple memory protection for low overhead costs. As such, when
allocating pages to the thread via the user allocator those pages will
be mapped to the protection domain of the currently running thread.
These protection domains are inserted into a single global page table,
that hold all the page table entries and the address space identifier
of the protected page.

In the memory protection subsystem, the default behaviour is to map all
the kernel pages (i.e. pages that are not in the user heap), to every
thread in the system as read only. This allows all threads to read from
kernel data, but prevents overwriting of that data.

TLB
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note::

   このセクションはMIPSプラットフォームにしか関係しません。

To facilitate memory protection, XINU uses the translation lookaside
buffer (TLB) built into the MIPS processors of the WRT54GL series of
routers. When a piece of software attempts to access memory in the
:ref:`user segment <mips_user_segment>`, a TLB load or store exception
will occur. When this occurs the processor jumps to a specific
exception handler which allows the kernel to look up the page table
entry, check if the faulting thread is in the same memory space as the
entry, and load the entry into the TLB. If there is no mapping or the
thread is not in the same address space, a memory protection violation
occurs and the thread is killed.
