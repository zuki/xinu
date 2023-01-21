はじめに
============

.. note::

    このドキュメントはEmbedded Xinuのソースコードとともに配布されて
    いるファイルから生成されたもので、現在 Embedded Xinu Wiki
    (http://xinu.mscs.mu.edu) を置き換えるものとして開発中です。

**Embedded Xinu** はオペレーティングシステムと組込みシステムの分野で
進行中の研究・実装プロジェクトです。当初の目的は :ref:`Xinuオペレーティングシステム <Xinu>` を再実装し、inksys WRT54GLルータなどの組み込み
MIPSプラットフォームに移植することでした。それ以後、Embedded Xinuは
:doc:`QEMU MIPSel仮想環境 <mips/Mipsel-qemu>` や
:doc:`Raspberry Pi </arm/rpi/Raspberry-Pi>` など、その他のプラット
フォームにも移植されています。:ref:`サポートプラットフォーム一覧 <supported_platforms>`
をご覧ください。Embedded Xinuは現在も開発中であり、新しいプラット
フォームへの移植も行われていますが、実験環境とカリキュラム教材は
すでにマーケット大学やその他の大学のオペレーティングシステムや
ハードウェアシステム、組込みシステム、ネットワーク、コンパイラの
授業で使用されています。

Embedded Xinuプロジェクトは `Dennis Brylow博士 <http://www.mscs.mu.edu/~brylow/>`__
が発案・指導し、ウィスコンシン州ミルウォーキーにある `マーケット大学 <http://www.mu.edu/>`__ の `数学・統計・コンピュータサイエンス  <http://www.mscs.mu.edu/>`__
学部の :doc:`システムラボラトリ <development/Systems-Laboratory>` で
大学院生と学部生が取り組んでいます。Embedded Xinuの最初の主要な作業は
2006年の夏に始まりました。

プロジェクトのパートナーにはバッファロー大学の
`Bina Ramamurthy博士 <http://www.cse.buffalo.edu/~bina/>`__
（ `NSF CCLI <http://www.nsf.gov/pubs/2009/nsf09529/nsf09529.html>`__
助成金を共有）、ミシシッピ大学の
`Paul Ruth博士 <http://cs.olemiss.edu/~ruth/wiki/doku.php>`__
、パデュー大学の `Doug Comer博士 <http://www.cs.purdue.edu/people/comer>`__
（Xinuの生みの親）がいます。

はじめよう
---------------

Embedded Xinuのダウンロード、コンパイル、実行を始めるには
:doc:`Getting-Started` を参照してください。

Embedded Xinuの機能については :doc:`features/index` を参照して
ください。

特定のプラットフォームに関する情報も提供しています。

- :doc:`/arm/index`
- :doc:`/mips/index`

Embedded Xinuを使用した教育
------------------------------

学部の授業にEmbedded Xinuを採用または適応させるためのカリキュラム
ガイダンスについては :doc:`teaching/index` を参照してください。

Embedded Xinuの実験環境の構築については :doc:`teaching/Laboratory`
を参照してください。

.. _supported_platforms:

サポートプラットフォーム
-------------------------

現在Embedded Xinuが動作しているすべてのプラットフォームの一覧です。

.. list-table::
    :widths: 10 10 17 8 8
    :header-rows: 1

    * - プラットフォーム
      - ステータス
      - コメント
      - :ref:`PLATFORM value <makefile_variables>`
      - :ref:`Cross-target <cross_compiler>`
    * - :doc:`Linksys WRT54GL <mips/WRT54GL>`
      - サポート済
      - 主たる開発プラットフォームであり、Xinuが徹底的にテストされています。
      - ``wrt54gl``
      - ``mipsel``
    * - Linksys WRT54G v8
      - サポート済
      - Embedded Xinuラボでテストと実行が行われています。WRT54GLと同じコードでサポートされています。
      - ``wrt54gl``
      - ``mipsel``
    * - Linksys WRT54G v4
      - おそらくサポート済
      - v4はWRT54GLのベースとなったバージョンらしいのでEmbedded Xinuラボで明示的にテストしていませんが、おそらく動作すると思われます。
      - ``wrt54gl``
      - ``mipsel``
    * - Linksys WRT160NL
      - サポート済
      - WRT54GLの新モデル。有線ネットワークインターフェイスを含む教育用O/Sコアが完全に機能します。
      - ``wrt160nl``
      - ``mips``
    * - Linksys E2100L
      - サポート済
      - 有線ネットワークインターフェイスを含む教育用O/Sコアが完全に機能します。
      - ``e2100l``
      - ``mips``
    * - ASUS WL-330gE
      - 活発には保守されていない
      - このプラットフォームはかつて動作していましたが、今では活発に保守、テストされていません。
      - ``wl330ge``
      - ``mipsel``
    * - :doc:`mipsel-qemu </mips/Mipsel-qemu>`
      - サポート済
      - 教育用O/Sコアが完全に機能しますが、ネットワークサポートは開発中です。
      - ``mipsel-qemu``
      - ``mipsel``
    * - :doc:`Raspberry Pi </arm/rpi/Raspberry-Pi>`
      - サポート済
      - 有線ネットワークを含むオペレーティングシステムのコアが機能します。
      - ``arm-rpi``
      - ``arm-none-eabi``
    * - :doc:`arm-qemu </arm/ARM-qemu>`
      - サポート済
      - 有線ネットワークを除くオペレーティングシステムのコアが機能します。
      - ``arm-qemu``
      - ``arm-none-eabi``

.. _Xinu:

オリジナルXinu
-----------------

オリジナルの **Xinu** (**"Xinu is not unix"**) は学生に
オペレーティングシステムの概念を教えるための小規模で学術的な
オペレーティングシステムです。1980年代初頭にパデュー大学で
Douglas E. Comer博士によりLSI-11 プラットフォーム向けに開発された
ものであり、現在では様々なプラットフォームに移植されています。

**Embedded Xinu** は学生にオペレーティングシステムの概念を教えると
いう当初の目的を維持しつつ、コードベースを現代化し、システムを
MIPSなどの最新のRISCアーキテクチャに移植することを試みている
このプロジェクトのアップデート版です。

.. note::
    この文書で単に"Xinu"または"XINU"と書かれていても、実際は
    ほとんどの場合、Embedded Xinuについて述べています。
