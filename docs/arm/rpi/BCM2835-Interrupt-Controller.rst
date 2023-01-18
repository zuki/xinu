BCM2835割り込みコントローラ
============================

**BCM2835割り込みコントローラ** は :doc:`Raspberry-Pi` で使用されている
:doc:`BCM2835` で利用可能なメモリマップドペリフェラルです。これにより
ソフトウェアが特定のIRQ（割り込み要求）を有効または無効にすることができます。
通常、各IRQはチップ上で利用可能な何らかのデバイスに対応しています。

ハードウェアの詳細
--------------------

BCM2835ではいくつかのIRQがARM CPUとVideoCore GPUで共有されていることを
理解することが重要です。この割り込みコントローラはこれらの共有IRQとARM固有の
IRQの **両方を** 制御しており、レジスタのレイアウトはこの分離を反映しています。
いくつかの共有IRQはGPUにより既に有効化されているので、有効化する必要は
ありません。しかし、この割り込みコントローラは、どの割り込みを実際にARMに
ルーティングするかを制御するためにARMが使用するだけであり、GPUは独自の
割り込みコントローラを持っている場合がほとんどです。

BCM2835割り込みコントローラは物理メモリアドレス0x2000B000で利用可能な
メモリマップドペリフェラルです。次の表は各レジスタを説明したものです。
レジスタは32ビットです。**オフセットは0ではなく、0x200から始まることに
注意してください。これはBroadcomのドキュメントとの一貫性を保つためです。
完全にクリアにするために、IRQ_basic_pending は物理メモリアドレス 0x2000B200 に
配置されています。**

.. list-table:: BCM2835割り込みコントローラレジスタ
    :widths: 5 15 30
    :header-rows: 1

    * - オフセット
      - 名称
      - 説明
    * - ``+0x200``
      - IRQ_basic_pending
      - 保留中のARM固有のIRQのビットマスクと割り込み処理を高速化するための
        追加ビット（現状、Embedded Xinuでは使用されていない）。
    * - ``+0x204``
      - IRQ_pending_1
      - 保留中の共有のIRQ 0-31のビットマスク
    * - ``+0x208``
      - IRQ_pending_2
      - 保留中の共有のIRQ 32-63のビットマスク
    * - ``+0x20C``
      - FIQ_control
      - TODO
    * - ``+0x210``
      - Enable_IRQs_1
      - 0-31の範囲の共有IRQの1つ以上を有効にするために対応するビットに
        1を書き込む
    * - ``+0x214``
      - Enable_IRQs_2
      - 32-63の範囲の共有IRQの1つ以上を有効にするために対応するビットに
        1を書き込む
    * - ``+0x218``
      - Enable_Basic_IRQs
      - ARM固有のIRQの1つ以上を有効にするために対応するビットに1を書き込む
    * - ``+0x21C``
      - Disable_IRQs_1
      - 0-31の範囲の共有IRQの1つ以上を無効にするために対応するビットに
        1を書き込む
    * - ``+0x220``
      - Disable_IRQs_2
      - 32-63の範囲の共有IRQの1つ以上を無効にするために対応するビットに
        1を書き込む
    * - ``+0x224``
      - Disable_Basic_IRQs
      - ARM固有のIRQの1つ以上を無効にするために対応するビットに1を書き込む

この割り込みコントローラを実際に使用するためにはIRQとデバイスのマッピングが
分かっていなかればなりません。次の表は私たちがEmbedded XinuでテストしたIRQを
記録した **不完全な** リストです。完全な表は `Linuxのヘッダーでの定義
<https://github.com/raspberrypi/linux/blob/rpi-3.6.y/arch/arm/mach-bcm2708/include/mach/platform.h>`__ に見ることができます。以下の表ではEmbedded Xinuと
Linuxで使用されている番号付け方式を使用しています。すなわち、共有のIRQは0-63
の番号を付け、ARM固有のIRQには64から始まる番号を付けています。

.. list-table:: 不完全なBCM2835 IRQ一覧
    :widths: 5 15 30
    :header-rows: 1

    * - IRQ
      - デバイス
      - 注記
    * - 0
      - System Timer Compare Register 0
      - このIRQを有効に **してはならない** ; GPUですでに使用されているため。
    * - 1
      - System Timer Compare Register 1
      - :doc:`BCM2835-System-Timer` を参照
    * - 2
      - System Timer Compare Register 2
      - このIRQを有効に **してはならない** ; GPUですでに使用されているため。
    * - 3
      - System Timer Compare Register 3
      - :doc:`BCM2835-System-Timer` を参照
    * - 9
      - USB Controller
      - USBデバイスとの通信はすべてUSBコントローラを介して行われるため、
        このIRQが **唯一の** USB IRQとなります。:doc:`Synopsys-USB-Controller` を
        参照
    * - 55
      - PCM Audio
      -
    * - 62
      - SD Host Controller
      -

注記:

- ソフトウェアは割り込みコントローラを使用して割り込みを「クリア」することは
  できません。割り込みはデバイス固有の方法でクリアする必要があります。

- 共有割り込みの中には、IRQ_Basic_PendingレジスタとIRQ_Pending_1または
  IRQ_Pending_2レジスタに現れるものがありますが、Enable_Basic_IRQsまたは
  Disable_Basic_IRQsで有効/無効にすることはできません。

Embedded Xinuでの使用
------------------------

Embedded Xinu (:doc:`XinuPi`) はBCM2835割り込みコントローラを使って
``enable_irq()`` と ``disable_irq()`` を実装しています。コードは
:source:`system/platforms/arm-rpi/dispatch.c` にあります。これらの関数には
有効または無効にするIRQの番号が渡されるだけです。共有IRQの番号は0-63であり。
ARM固有のIRQ (現在は実際には使用されていません) は64から始ま番号です。この
ファイルには ``dispatch()`` もあります。この関数は割り込みを処理するために
:source:`system/arch/arm/irq_handler.S` にあるアセンブリ言語で書かれたIRQ
ハンドラから呼び出されます。 ``dispatch()`` の目的は実際にどの番号のIRQが
保留されているかを把握し、そのIRQを処理する登録済みの割り込みハンドラを呼び
出すことです。

外部リンク
--------------

- `BCM2835 ARM Peripherals datasheet by Broadcom
  <http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf>`__
  割り込みコントローラはセクション7 (p.109-118) に記載されています。
  Raspberry Piの他のハードウェアと比較するとこのコンポーネントはドキュメントが
  充実している部類に入ります。ただし、Broadcom のドキュメントには0-3 (システム
  タイマー)や9(USB コントローラ) などの重要なIRQ番号について言及されていない
  ことに注意してください。
