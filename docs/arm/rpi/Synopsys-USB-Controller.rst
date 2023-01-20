Synopsys DesignWare High Speed USB 2.0 On The Go Controller
===========================================================

**Synopsys DesignWare High-Speed USB 2.0 On-The-Go Controller** は
:doc:`Raspberry-Pi` で使用されているUSBコントローラです。このハード
ウェアはエンドユーザが利用できる公式ドキュメントがないこと [#nodocs]_ 、
非常に複雑で記述が不十分なLinuxドライバがあることで悪名高いです。
LinuxのUSBサブシステムのメンテナであるGreg Kroah-Hartmanによると [#linux_maintainers]_:

    「...それは本当にひどいUSBコントローラチップであり、それを
    プロセッサに接続する悲しい方法と本当にひどいドライバとが相まって
    このボードでUSBがとにかく機能するという事実はまったく奇跡です」
    [#usb_sucks_quote]_

このページではEmbedded XinuをRaspberry Piに移植するプロジェクトの
一環として私たちが書いたUSBホストコントローラドライバの文脈でこの
ハードウェアの説明を試みています。残念ながらこのハードウェアを
*完全に* にドキュメント化することは意図していません。なぜなら、
Embedded XinuのドライバはLinuxのドライバが持つ多くの機能をサポート
しない比較的単純で簡素化されたドライバだからです。

Embedded Xinuドライバの概要
----------------------------------

すでに述べたようにこのUSBコントローラに関するドキュメントは存在しません。
そのため、ドライバの実装が困難であることは明らかです。しかし、他に選択肢は
ないのでこのコントローラ用に書かれた他のドライバ、主にLinuxドライバ
[#linux_driver]_ ですが、CSUDドライバ [#csud]_ やPlanドライバ
[#plan9_driver]_ などその他のコントローラ用に書かれたドライバから
関連するハードウェアの詳細を引き出す必要がありました。ただし、私たちの
コードはシンプルで文書化されていること、また、シンプルな教育用
オペレーティングシステムに含めるのに（USBとこのハードウェア用に
可能な範囲で）適切であることを意図した新しい実装です。

Embedded Xinuのドライバはコントロール、インターラプト、バルクの3つの
転送をサポートしています。ホストコントローラドライバとして
:source:`include/usb_hcdi.h` で宣言されているインターフェイスを実装
しています。簡略化のためにEmbedded XinuのドライバはLinuxドライバの
（以下に示す、ただし、これに限定されない）一部の機能をサポート
**していません** 。


-  デバイスモード。ハードウェアの名前にある"On-The-Go"は `On-The-Go
   プロトコル <https://en.wikipedia.org/wiki/USB_On-The-Go|USB>`__ を
   サポートしていることを意味します。これは主たるUSB仕様の拡張であり、
   USBハードウェアが「ホスト」モードと「デバイス」モードのいずれでも
   動作できるようにするものです。しかし、このドライバでは、ホストモード
   だけを対象としています。
-  アイソクロナス転送
-  Raspberry Piで使用されている以外のシリコンのインスタンス化のサポート
-  周期的転送の特別な特性を考慮した高度なトランザクションスケジューリング
-  ドライバを構成するための様々なモジュールパラメータ
-  サスペンドとハイバネーションを含む電源管理
-  スレーブまたはディスクリプタDMAモード

さらなる詳細
------------

デバイスとレジスタについてのさらなる詳細はいずれここに追加されるかも
しれません。今のところはソースコード(:source:`system/platforms/arm-rpi/usb_dwc_hcd.c` と
:source:`system/platforms/arm-rpi/usb_dwc_regs.h`) を見てください。
コードはやさしく読めて、きちんと解説することを心がけています。USB自体が
非常に複雑なので「読みやすく」するにも限界があるのは明らかですが。

注記
-----

.. [#nodocs] http://www.raspberrypi.org/phpBB3/viewtopic.php?f=72&t=27695
.. [#linux_maintainers] http://lxr.linux.no/linux/MAINTAINERS
.. [#usb_sucks_quote] http://lists.infradead.org/pipermail/linux-rpi-kernel/2012-September/000214.html
.. [#linux_driver] https://github.com/raspberrypi/linux/tree/rpi-3.6.y/drivers/usb/host/dwc_otg
.. [#csud] https://github.com/Chadderz121/csud
.. [#plan9_driver] http://plan9.bell-labs.com/sources/plan9/sys/src/9/bcm/usbdwc.c
