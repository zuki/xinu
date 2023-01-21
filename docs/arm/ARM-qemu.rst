arm-qemu
========

|EX| は `QEMU <http://qemu.org>`__ が提供しているARM仮想環境に
移植されています。具体的には、ARM1176 CPUとARM Versatile Platform Baseboardのペリフェラルを組み合わせたエミュレーション環境が
サポートされています。このペリフェラルは2つのARM Dual Timer Module
（SP804）、PrimeCell Vectored Interrupt Controller（VIC）（PL011）、
4つの PrimeCell UART（PL011）とその他のデバイスで構成されています。
:doc:`/mips/Mipsel-qemu` ポートと同様に、Embedded Xinuの *arm-qemu*
ポートは「本物の」ハードウェアを用意することなくRISCアーキテクチャ
上で基本的なEmbedded Xinu環境を簡単に実行する方法を提供します。

ビルド
--------

``PLATFORM=arm-qemu`` で :ref:`Embedded Xinuのコンパイル <compiling>`
をします。この場合、ARMをターゲットとした :ref:`クロスコンパイラ <cross_compiler>` が必要であることに注意してください。 ``compile/``
ディレクトリに``xinu.boot`` というファイルが生成されます。

実行
-------

::

    $ qemu-system-arm -M versatilepb -cpu arm1176 -m 128M -nographic -kernel xinu.boot

``-cpu arm1176`` オプションに関する注記。実行にはQEMU v1.0 (released
December 2011) 以降が必要です。

注記
-----

``arm-qemu`` プラットフォームはまだネットワークをサポートしていません。
