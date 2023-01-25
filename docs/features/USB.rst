USB
===

**USB (Universal Serial Bus)** is a standard for connecting devices to a
computer system. It supports an immense range of devices, including (but
not limited to) keyboards, mice, flash drives, microphones, and network
adapters.

Although USB is ubiquitous in modern computer systems, even in some
"embedded" devices, it is very challenging to implement software support
for USB. This is primarily a result of the high complexity of USB, which
arises from several factors, including support for virtually any
arbitrary device, support for dynamic device attachment and detachment,
backwards compatibility with multiple versions of the USB specification,
and multiple supported speeds and transfer types. The USB 2.0
specification is 650 pages long, yet only covers a fraction of the
information needed to implement, from scratch, a USB software stack and
a driver controlling a specific USB device.

Due to the high complexity of USB, this article cannot fully explain
USB, nor can it even fully explain Embedded Xinu's implementation of
USB. Instead, it gives an overview of USB in the context of Embedded
Xinu's implementation. For full details about USB, the reader will
inevitably need to read the USB specification itself, as well as other
relevant specifications and webpages. For full details specifically
about Embedded Xinu's implementation, the reader will inevitably need to
read the source code.

.. contents::
   :local:

一般的なUSBに関する情報
-----------------------

バストポロジ
~~~~~~~~~~~~

Fundamentally, USB is just a way to connect devices to a computer
system. A USB bus accomplishes this by arranging devices in a tree. Each
node of the tree is a **USB device**. There are two fundamental types of
USB devices: **hubs** and **functions**. USB hubs can have "child"
devices in the tree, while functions cannot. Hubs can be "children" of
other hubs, up to a depth of 7 levels.

The root node of the tree is the **root hub**, and every USB bus has one
(although it may be faked by the Host Controller Driver, described
later).

A USB hub provides a fixed number of attachment points for additional
devices called **ports**. A USB port may be either completely internal
or exposed to the "outside" as a place to plug in a USB cable. From the
user's point of view there is certainly a difference between these two
manifestations of a USB port, but from the software's point of view
there is no difference. On a similar note, it is also possible that a
single physical package, which a naive user might refer to as a "USB
device", actually contains an integrated USB hub onto which one or more
USB devices (as defined above) are attached. Such physical packages are
referred to as **compound devices**. An example of a compound device is
one of Apple's USB keyboards that provides a USB port to attach a mouse.

Since USB is a dynamic bus, USB devices can be attached or detached from
the USB at any arbitrary time. Detaching a hub implies detaching all
child devices.

For its part, Embedded Xinu's USB implementation fully supports the
dynamic tree topology of a USB bus.

デバイス
~~~~~~~~~

Due to the generality of USB a USB device that is not a hub can be
virtually anything at all. This is made possible in part by a highly
nested design:

-  A USB device has one or more **configurations**.
-  A configuration has one or more **interfaces**.
-  An interface has one or more **alternate settings**.
-  An alternate setting has one or more **endpoints**.

Every device, configuration, interface, and endpoint has a corresponding
**descriptor** that can be read by the USB software to retrieve
information about the described entity in a standard format.

Although this format allows for highly complex devices, most devices are
relatively simple and have just one configuration. Furthermore, common
devices only have one interface. In fact, as of this writing, Embedded
Xinu's USB subsystem aims to support the common case only; it therefore
always sets the device to its first listed configuration, then attempts
to bind a device driver to the entire device rather than examining
individual interfaces to see if they need separate "interface drivers".

ホストコントローラ
~~~~~~~~~~~~~~~~~~~~

USB is a polled bus, so all transfers over the USB are initiated by the
**host**. The term "host" in this context means the USB software as well
as the USB Host Controller, which is the hardware responsible for
actually sending and receiving data over the USB and maintaining the
root hub. This is actually one of the trickier parts of USB. Since the
USB specification itself does not standardize the exact division of
tasks between software and hardware, it's often not clear who is
responsible when the specification says "host".

The essential thing to know is that the place where the USB software
directly meets the USB hardware is in the USB Host Controller Driver,
which operates the USB Host Controller. Some USB Host Controllers
present standard interfaces (such as UHCI, OHCI, or EHCI--- all
defined in specifications separate from the USB specification itself)
to software. Others do not present a standard interface, but instead
have vendor-provided documentation and/or a vendor-provided driver; an
example of this is the :doc:`/arm/rpi/Synopsys-USB-Controller` used on
the :doc:`/arm/rpi/Raspberry-Pi`.  Obviously, a standard interface is
highly preferred when independently implementing a Host Controller
Driver.

転送
~~~~~~~~~

To communicate with USB devices, the host sends and receives data over
the USB using USB transfers. A USB transfer occurs to or from a
particular endpoint on a particular device. Every endpoint is associated
with a specific type of USB transfer, which can be one of the following:

-  **Control** transfers. These are typically used for device
   configuration. There are two main unique features of these transfers.
   First, a special packet called SETUP is always sent over the USB
   before the actual data of the control transfer, and software needs to
   specify the contents of this packet. Second, every device has an
   endpoint over which control transfers in either direction can be
   made, and this endpoint is never explicitly listed in the endpoint
   descriptors.
-  **Interrupt** transfers. These are used for time-bounded transmission
   of small quantities of data (e.g. data from a keyboard or mouse).
-  **Bulk** transfers. These are used for reliable (with error
   detection) transmission of large quantities of data with no
   particular time guarantees (e.g. reading and writing data on mass
   storage devices).
-  **Isochronous** transfers. These are used for regular transmission of
   data with no error detecting (e.g. video capture).

現在、Embedded Xinuはコントール転送、インターラプト転送、バルク転送を
サポートしています。アイソクロナス転送はまだテストされていません。また、
インターラプト転送は機能しますが、USBの仕様で要求されている時間的制約の
ある転送を保証するためにもう少し作業が必要かもしれません。

速度
~~~~~~

USB supports multiple transfer speeds:

-  1.5 Mbit/s (Low Speed) (USB 1+)
-  12 Mbit/s (Full Speed) (USB 1+)
-  480 Mbit/s (High Speed) (USB 2.0+)
-  5000 Mbit/s (Super Speed) (USB 3.0+)

Yes, Full Speed is in fact the second lowest speed. Well I think we all
know that 12 Mbit/s ought to be enough for anyone. But anyway, due to
the need to maintain backwards compatibility with legacy devices, the
USB software (mainly the host controller driver) unfortunately needs to
take into account transfer speeds. At minimum, it must be aware that
transfers to or from devices attached at Low Speed or Full Speed are
performed as a series of **split transactions**, which allow Low Speed
or Full Speed transfers to occur without significantly slowing down the
portion of the USB bus operating at a higher speed.

これを書いている時点では、Embedded XinuのUSBサブシステムはUSB 2.0を
サポートしており、LS、FS、HSで動作するデバイスをサポートしています。
USB 3.0のSSはサポートされていません。

.. _usb_subsystem:

Embedded XinuのUSBサブシステム
---------------------------------

USBに関する一般的な情報を示したので、USBソフトウェアスタックの
基本設計を理解することは容易でしょう。以下の説明は、確かにコードを
編成する唯一の方法ではありませんが、ほとんどのオペレーティング
システムで使用されている方法であり、USBが設計思想に基づいたもっとも理に
かなったものです。Embedded Xinuの観点からおそらく第一の疑問は、なぜUSB
デバイスとUSBコントローラはデフォルトでは他のEmbedded Xinuデバイスのように
``devtab`` にデバイスとして表示されないのかです。その理由は、USBは動的な
バスなので静的なテーブルでは記述できないこと、USBデバイスが高度なネスト
構造をとること、複数の転送タイプがサポートされていることにより、
単純な「デバイスからの ":source:`read() <system/read.c>`, :source:`write() <system/write.c>`"」パラダイムには複雑すぎるからです。

.. note::
    必要であれば、特定のUSBデバイスドライバを ``devtab``  にデバイス
    エントリに提供することはできます。ただし、物理デバイスは依然として
    ホットプラグ可能であることを考慮しなければなりません。

.. note::

    すべてのEmbedded Xinu :ref:`platforms <supported_platforms>` が
    USBをサポートしているわけではありません。USBハードウェアが利用
    できないか、適切なUSBホストコントローラドライバが実装されていない
    ためです。

.. _usb_components:

構成要素
~~~~~~~~~~

-  **USBホストコントローラドライバ** はプラットフォーム固有のホスト
   コントローラハードウェアを利用して、USB上で実際にデータを送受信する
   役割を担っています。このドライバの目的は、USBホストコントローラの違いを
   USBを扱う他のすべてのコードから切り離すことです。Embedded Xinuでは
   USBホストコントローラドライバは :source:`include/usb_hcdi.h` で宣言
   されているインターフェイスを実装する必要があります（ただし、これを
   書いている時点では、実装されているホストUSBコントローラドライバは1つ
   だけで、 :doc:`/arm/rpi/Raspberry-Pi` で使用されている
   :doc:`/arm/rpi/Synopsys-USB-Controller` を制御するものです）。
-  **USBコアドライバ** はツリー構造を含むUSBデバイスモデルの維持と
   USBデバイスドライバを記述するためのフレームワークを提供する役割を
   担っています。ホストコントローラドライバを直接使用するよりもUSB
   デバイスドライバの開発を容易にする多くの便利な機能を提供しています。
   これはプラットフォーム固有のホストコントローラドライバをできるだけ
   分離するための試みであると考えられます。また、デバイスの構成や
   アドレスの設定、ディスクリプタの読み込みなど、すべてのUSBデバイスに
   共通する設定も扱います。Embedded XinuのUSB Core Driverは
   :source:`device/usb/usbcore.c` にあります。
-  **USBデバイスドライバ** は特定のUSBデバイスを制御する役割を担って
   います。USBは動的なバスなので、USBデバイスドライバはUSBコアドライバの
   助けを借りて実行時に実際のUSBデバイスにバインドされます。すべてのUSB
   ソフトウェアスタックで必ず実装されなければならない非常に重要なSUB
   デバイスドライバが **USBハブドライバ** です。このドライバは、USBハブの
   状態の監視とデバイスの着脱をUSBコアドライバに報告する役割を担っています。
   Embedded XinuのUSBハブドライはたとえば。 :source:`device/smsc9512/`
   のように  :source:`device/` にあります。

.. note:: Linuxのスタックのようなより完全（で複雑）なUSBソフトウェア
          スタックでは、USBデバイスではなくUSBインタフェースに関連する
          **USBインタフェースドライバ** もサポートされています。

Embedded XinuのUSBサポートの有効化
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Embedded Xinuの特定のビルドにUSBのサポートを含めるには
 ``xinu.conf`` で ``WITH_USB`` を定義し、 ``platformVars`` の
 ``DEVICES`` 変数に  ``usb`` を追加してください。USBハブドライバは
 USBがあらゆるデバイスをサポートするために必要なものなので自動的に
 含まれることに注意してください。

さらに、プラットフォームに固有でこのディレクトリには存在しない
適切なホストコントローラデバイスを書いて、カーネルにコンパイルする
必要があります。たとえば、 :source:`system/platforms/arm-rpi/usb_dwc_hcd.c`
はRaspberry Piハードウェアで使用されるホストコントローラデバイスです。

最後に、サポートしたい実際のUSBデバイスを対応するデバイスディレクトリを ``platformVars`` の ``DEVICES`` 変数に追加し、 ``xinu.conf`` で適切な
スタティックデバイスを定義することにより有効にする必要があります。たとえば、Raspberry Piでは :source:`device/smsc9512` にあるSMSC LAN9512 USB Ethernet
Adapterのドライバを有効にするために ``DEVICES`` に ``smsc9512`` を追加し、
``xinu.conf`` で ``ETH0`` デバイスを定義しています。

組込みシステム用のUSB
~~~~~~~~~~~~~~~~~~~~~~~~

デバッグ機能が重要でない完全な組み込みシステムの場合、不要なヒューマン
フレンドリ機能はUSBコアから省略することができます。詳細については
:source:`device/usb/usbdebug.c` を参照してください。

USB関連のシェルコマンド
~~~~~~~~~~~~~~~~~~~~~~~~~~

**usbinfo** :doc:`シェルコマンド <Shell>` は、USBに接続されたデバイスの
情報を表示します。詳細は :source:`shell/xsh_usbinfo.c` を参照するか、
``usbinfo --help`` を実行してください。

.. _how_to_write_usb_device_driver:

USBデバイスドライバの書き方
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

まず、USBデバイスのドキュメントを入手する必要があります。多くのデバイスは
USBの **クラス仕様** に準拠しているため独自のドキュメントを持っていない
ことに注意してください。クラス仕様に準拠している場合、ドキュメントはクラス
仕様書です。ただし、通常、これらは非常に長くて複雑です。

ドキュメントがない非標準デバイスの場合は、他のオペレーティングシステムの
ソースコードなど、デバイスのプロトコルを理解するために利用可能なあらゆる
手段を使用する必要があります。最後の手段として、バイナリドライバが生成する
USBトラフィックを盗み見ることによりUSBデバイスのソフトウェアインタフェースを
リバースエンジニアリングすることができます。

いずれにせよ、ドライバを書くためには、デバイスとやり取りされるメッセージの
形式と意味、そしてそれらがどのUSBエンドポイントと転送タイプに関連付けられて
いるかを理解する必要があります。

例:

- マウスウスなどのUSBヒューマンインタフェースデバイスはマウス座標などの
  入力データの報告に使用されるINインターラプトエンドポイントが必要であり、
  ある種のメタデータはデフォルトのコントロールエンドポイントから問い
  合わせることができます。
- :doc:`/arm/rpi/SMSC-LAN9512` のようなUSBネットワークデバイスは
  ネットワークパケットを受信するためのバルクINエンドポイントと
  ネットワークパケットを送信するためのバルクOUTエンドポイントを提供します。

コードそのものについては、Embedded XinuではUSBデバイスドライバは
:source:`usb_core_driver.h` で宣言されているUSBコアドライバが提供する
APIを使って実装されます。このAPIによりドライバはドライバ自身の登録、
コアによって検出されるUSBデバイスへのバインド、USBデバイスとの通信が
可能になります。これらについてはソースコードに非常に詳しく記述されて
います。また、USBデバイスドライバの例については
:source:`device/smsc9512/` を参照してください。

Xinuの静的デバイスモデルはUSBの動的デバイスモデルとは互換性がない
ことに注意してください。そのためUSBデバイスドライバで回避策が必要な
場合があります。たとえば、ドライバは一定数以上のUSBデバイスとの
バインドを拒否するかもしれませんし、USBデバイスに実際にバインド
される前にコードが静的デバイスを開こうとするとブロックしたり失敗を
返したりするかもしれません。

USBホストコントローラドライバの書き方
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Embedded Xinuでは、USBホストコントローラドライバは実際にハードウェア
（USBホストコントローラ）と対話し、USB上でデータを送受信する役割を担って
います。残念ながら、USBホストコントローラはUSB仕様では標準化されておらず、
それがこの抽象化レイヤが必要である理由となっています。USBホスト
コントローラにはUHCI、OHCI、EHCIの各仕様に準拠したものもあれば、
:doc:`/arm/rpi/Raspberry-Pi` に使用されている
:doc:`/arm/rpi/Synopsys-USB-Controller` のような非標準のものもあります。

最初のステップは、Xinuが対象となるハードウェアのUSBホストコントローラを
すでにサポートしているか否かを確認することです。もししていればそのコードを
使うことができますが、変更が少し必要でしょう（たとえば、メモリマップド
レジスタの位置など）。そうでない場合は、続きをお読みください。

USBホストコントローラドライバは :source:`include/usb_hcdi.h` で宣言
されているインタフェースを実装する必要があります。

まず、（もしあれば）ホストコントローラのドキュメントを入手する必要が
あります。また、USB2.0仕様の関連部分（主にコントロール、インターラプト、
バルクの各転送について記述されている部分）を読む必要があります。
650ページのほとんどは読む必要が **ありません** 。

次に、``hcd_start()`` にホストコントローラを使用可能な状態にするために
必要なコードを書く必要があります。

次の、そして本質的に最後のステップは ``hcd_submit_xfer_request()`` の
実装ですが、これは非常に難しいです。最初は、ルートハブに送信される
偽のリクエストに焦点を当てるべきです。これにはルートハブのデフォルト
エンドポイントとの間の様々なコントロール転送とルートハブのステータス
変更エンドポイントからのインターラプト転送が含まれます。ルートハブの
リクエストにはソフトウェアですべて処理できるものと、ホストコントローラと
通信する必要があるものがあります。次に、バス上の実際のUSBデバイスとの
間のコントロール転送をサポートする必要があります。最後に、インターラプト
転送とバルク転送をサポートする必要があります。これらは、非同期かつ
割り込み駆動である必要があります。ハブドライバはポートの状態変化を
検出するためにインターラプト転送を使用しますので、インターラプト転送を
実装しないとUSB全体のエヌメレーションをすることができないことに注意
してください。

デバッグメッセージを表示するために ``usb_debug()`` マクロと
``usb_dev_debug()`` マクロを使用することができます。有効にするには
:source:`include/usb_util.h` にあるログ優先順位を変更してください。

さらなる読み物
---------------

- `USB 2.0 Specification <http://www.usb.org/developers/docs/>`__
- `USB 3.1 Specification <http://www.usb.org/developers/docs/>`__
- Embedded Xinu USB 2.0 サブシステム. (:source:`device/usb`)
- Embedded Xinu USB デバイスドライバ. (例: :source:`device/smsc9512/`)
- Embedded Xinu USB ホストコントローラドライバ. (例: :source:`system/platforms/arm-rpi/usb_dwc_hcd.c`)
