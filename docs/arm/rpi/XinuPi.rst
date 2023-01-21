XinuPi
======

**XinuPi** はEmbedded Xinuを :doc:`Raspberry-Pi` に移植したものです。
XinuPiはRaspberry Piのためのシンプルで軽量なオペレーティングシステムを
提供し、そのコード量は通常デバイス上で動作するLinuxベースのソフト
ウェアスタックよりも数桁少ないものになっています。その目的は大学レベルの
コンピュータサイエンス教育の様々な分野、たとえば、OS、組み込みシステム、
ネットワーク、プログラミング言語などに安価で便利なプラットフォームを
提供することです。XinuPiのもう一つの目的は、Raspberry Piのこれまで
ドキュメントが乏しかったり、ドキュメントがなかったハードウェアの
ドキュメント化を行うことです。これにはこのドキュメントやXinuPiのソース
コード、コード内のコメントから生成されたドキュメントが含まれます。

Raspberry Piハードウェアの入手
-----------------------------------

ハードウェア自体に関する情報は :doc:`Raspberry-Pi` を参照してください。

.. _xinupi_getting_started:

XinuPiのダウンロード、コンパイル、実行
------------------------------------------

Embedded Xinuのダウンロードとコンパイルについては :doc:`/Getting-Started`
に記載されています。Raspberry Pi用にコンパイルする（"XinuPi"をビルドする）
ためには **make** の際に ``PLATFORM=arm-rpi`` を指定する必要があります。
また、ARMクロスコンパイラ（たとえば、 ``--target=arm-none-eabi`` でビルド
したbinutilsとgcc）が必要です。詳細は :ref:`cross_compiler` を参照して
ください。

注: 現在のところ、Raspberry Piのサポートは開発版（git）のみで、リリース
版のtarballにはありません。

コンパイルの結果、 ``compile/xinu.boot`` というファイルが生成されるので
これを ``kernel.img`` という名前でRaspberry PiのSDカードにコピーすれば
実行できます（ :ref:`raspberry_pi_booting` を参照）。

.. _xinupi_features:

XinuPiの機能と実装
----------------------------------

-  XinuPiのコアはRaspberry Piのためのプリエンプティブマルチタスクな
   オペレーティングシステムを提供します。Embedded XinuがRaspberry Piの
   ようなARMベースのプラットフォームでプリエンプティブマルチタスクを
   実装する方法の詳細については :doc:`/arm/ARM-Preemptive-Multitasking`
   を参照してください。これにはスレッドの生成とコンテキストスイッチに
   ついての情報が含まれています。また、XinuPiがプリエンプティブマルチ
   タスクの実装に使用しているRaspberry Piのタイマーについては
   :doc:`BCM2835-System-Timer` を参照してください。

-  タイマー割り込みやその他多くのデバイスに必要なRaspberry Piにおける
   割り込み処理については :doc:`Raspberry-Pi-Interrupt-Handling` で
   説明されています。

-  Embedded XinuにはUSBのサポートが一部追加されています。Raspberry Pi
   Model BにはEthernet Controllerが内蔵されているなど、Raspberry Piで
   重要な役割を担っているからです。USBに関する一般的な情報はUSBを、特に
   Raspberry Piが提供するUSBコントローラに関する情報については
   :doc:`Synopsys-USB-Controller` を参照してください。

-  Raspberry Pi Model Bに内蔵されているUSBイーサネットアダプタとその
   XinuPiのドライバについては :doc:`SMSC-LAN9512` を参照してください。
