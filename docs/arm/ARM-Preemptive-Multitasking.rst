プリエンプティブマルチタスク (ARM)
======================================

このページではEmbedded Xinuを :doc:`rpi/Raspberry-Pi` などの
:wikipedia:`ARM architecture` プラットフォームに移植する際に
:doc:`プリエンプティブマルチタスク </features/Preemptive-Multitasking>`
をどのように実装したかを説明します。

スレッドコンテキスト
----------------------

Embedded XinuのARMポートで使用した :ref:`スレッドコンテキスト <thread_context>`
のフォーマットとサイズは2つの要因: ARMアーキテクチャで使用可能な
レジスタとARMの標準的な呼び出し規約 [#calling]_ に依存しています。

-  ARMプロセッサには ``r0`` から ``r15`` までの16個の「汎用」
   レジスタと ``cpsr`` (current Program Status Register) レジスタが
   あります。ただし、「汎用」レジスタではありますが実際には ``r13``
   から ``r15`` には特別な用途があり、``r13`` はスタックポインタ
   (``sp``)、 ``r14``  はリンクレジスタ (``lr``)、 ``r15``  は
   プログラムカウンタ (``pc``) として使用されます。

-  標準的なARM呼び出し規約では、callee-saveのレジスタ(``r4`` -
   ``r11``, ``r13`` - ``r14``)とcaller-saveのレジスタ (``r0`` -
   ``r3``, ``r11``)、プロシージャへの引数の渡し方（最大４つまでは
   ``r0`` - ``r3`` で、残りの引数はスタック渡し）が規定されています。

私達が選んだスレッドコンテキストは15ワード長で以下のとおりです。

==========     ========    ===========
オフセット     レジスタ    注記
==========     ========    ===========
``0x00``       ``r0``      第1スレッド引数, caller-save
``0x04``       ``r1``      第2スレッド引数, caller-save
``0x08``       ``r2``      第3スレッド引数, caller-save
``0x0C``       ``r3``      第4スレッド引数, caller-save
``0x10``       ``r4``      Callee-save
``0x14``       ``r5``      Callee-save
``0x18``       ``r6``      Callee-save
``0x1C``       ``r7``      Callee-save
``0x20``       ``r8``      Callee-save
``0x24``       ``r9``      Callee-save
``0x28``       ``r10``     Callee-save
``0x2C``       ``r11``     Callee-save
``0x30``       ``cpsr``    Current program status register
``0x34``       ``lr``      リンクレジスタ
``0x38``       ``pc``      プログラムカウンタ
==========     ========    ===========

ARM版の ``ctxsw()`` は :source:`system/arch/arm/ctxsw.S` で実装
されており、詳しい説明がされておりますのでここでは割愛します。

プリエンプション
----------------------

:ref:`プリエンプション <preemption>` 用のタイマ割り込みを発生させる
手段はARMアーキテクチャでは標準ではないため、:doc:`rpi/BCM2835-System-Timer` などのボード固有またはチップ固有のデバイスを
利用する必要があります。

注記
-----

.. [#calling] http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042e/IHI0042E_aapcs.pdf
