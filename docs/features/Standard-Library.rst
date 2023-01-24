標準ライブラリ
==================

.. contents::
   :local:

概要
--------

XINUのCライブラリである **libxc** はXINUとともに配布されれている
"最小限の" 標準Cライブラリです。高性能や標準の完全準拠ではなく、
理解しやすいことを意図しています。ただし、実装されている機能は
以下に示すものを除き、標準版とほとんど同じです。

API
---

冗長性を減らすため、libxcが提供する完全なAPI（関数とマクロ）は
このページには記載されていません。代わりに :source:`lib/libxc`
ディレクトリに実装されているすべての関数にはその動作を説明する
詳細なコメントが付けられています。このディレクトリにあるすべての
Cソースファイルは外部から個別に見える関数を実装していることに
注意してください。

ライブラリヘッダーは次のとおりです。

=======================================   ==========================
ヘッダー                                  説明
=======================================   ==========================
:source:`ctype.h <include/ctype.h>`       文字型
:source:`limits.h <include/limits.h>`     数値型の制限
:source:`stdarg.h <include/stdarg.h>`     可変引数リスト
:source:`stdint.h <include/stdint.h>`     固定幅整数型
:source:`stdio.h <include/stdio.h>`       標準入出力
:source:`stdlib.h <include/stdlib.h>`     標準ライブラリ定義
:source:`string.h <include/string.h>`     文字列操作
=======================================   ==========================

:source:`stddef.h <include/stddef.h>` も存在し、標準の``offsetof``,
``size_t``, ``NULL`` に加え、XINU固有の型もいくつか定義しています。

標準的な動作との違い
---------------------------------

様々な理由（通常は単純化のため）のためlibxcは以下の点で他のCライブラリ
とは異なります。

- 多くの標準関数（といくつかのヘッダー）が単に実装されていません。
  たとえば、浮動小数点数数学関数、``setjmp()`` と ``longjmp()``、
  ワイドキャラクタのサポート、ロケールのサポート、時間関数、
  複素数演算です。

- 書式付きのプリントとスキャンは限られた範囲の書式指定子しか
  サポートしていません。詳細は :source:`_doscan() <lib/libxc/doscan.c>`  と  :source:`_doprnt() <lib/libxc/doprnt.c>` を参照してください。

- :source:`include/ctype.h` で宣言されているctype関数はC99で規定
  されている  ``EOF`` (end-of-file) を扱いません。

- ``putc()`` は libxcでは実装されていません。Xinuではこれは「システム
  コール」であり、その引数は標準の ``putc()`` とは逆になっています。
  標準的な動作を得るには ``fputc()`` か ``putchar()`` を使用してください。

- stdio関数は標準的な実装のような出力のバッファリングをしません。
  代わりに（ ``FILE`` ストリームではなく）デバイスディスクリプタに
  直接書き込みます。

- :source:`strlcpy() <lib/libxc/strlcpy.c>` は、技術的には非標準の
  BSD拡張ですが、実装されています。これはXINUのいくつかの場所で
  :source:`strlcpy() <lib/libxc/strlcpy.c>` に相当する動作を期待して
  いたのに、誤って :source:`strncpy() <lib/libxc/strncpy.c>` を呼び
  出していたためです。

.. _libxc_overrides:

プラットフォーム固有のオーバーライド
----------------------------------------

ある種の関数（通常は、:source:`memcpy() <lib/libxc/memcpy.c>` や
:source:`strlen() <lib/libxc/strlen.c>` などの文字列関数）を特定の
アーキテクチャ向けにアセンブリ言語で書いた最適化した実装でCライブラリを
構築したい場合があります。これはXINUの趣旨に沿うものですが、コードの
理解や、ある関数があるプラットフォームで実際にどこで定義されているかを
見つけることが難しくなるため、推奨していません。

それでもなおこれを行いたい場合は、絶対に必要でない限り、libxc自体の
コードは変更しないでください。代わりにプラットフォーム固有の
platformVarsファイル（たとえば ``compile/platforms/$(PLATFORM)/platformVars`` ）
で変数 ``LIBXC_OVERRIDE_CFILES`` を定義してください。これはコンパイル
してはいけないlibxc内のCソースファイルのリストです。たとえば、
``memcpy()`` をオーバーライドする場合はplatformVarsで次のように
指定します。

    LIBXC_OVERRIDE_CFILES := memcpy.c

この場合、対応する関数の実装を提供する必要がありますが、ここではなく
プラットフォーム固有のディレクトリ（たとえば ``system/platforms/$(PLATFORM)`` ）で行ってください。

この方法には置き換えた関数が  ``libxc.a`` に含まれず、カーネルに
全体としてしか含まれないという制限があります。ただし、XINUでは
すべてが一つのカーネルイメージにリンクされのでこれは重要なことでは
ありません。

参考資料
----------

- :wikipedia:`C standard library - Wikipedia <C standard library>`
- `C99 standard <http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf>`_
-  Brian Kernighan and Dennis Ritchie. *The C Programming Language*,
   second edition. Prentice Hall.
