ルーティング
=============

:doc:`ネットワークサブシステム <index>` の一部として、XINUはIPv4
:wikipedia:`routing` をサポートしています。

ルーティングモジュールの中心は *ルーティングデーモン* です。これは
:source:`rtRecv() <network/route/rtRecv.c>` を使ってパケットを置く
事ができる :doc:`メールボックス </features/Mailboxes>` からパケットを
繰り返し取得し :source:`rtSend() <network/route/rtSend.c>` を呼び出す
ことでパケットを宛先にルーティング（または :doc:`ICMP` 未到達パケット
などの特殊パケットを送信）します。パケットをルーティングするために
ルーティングデーモンはルーティングテーブルを使用します。ルーティング
テーブルのエントリは各々 :source:`rtAdd() <network/route/rtAdd.c>` と
:source:`rtRemove() <network/route/rtRemove.c>` を使ってプログラムで
追加・削除することができます。シェルコマンドの **route** はこの2つの
関数を使用しています。

ルートの追加
-----------------

ルートテーブルエントリテーブルにルートを追加するには、次のパラメタと
ともに **route add** を使用します。

.. code-block:: none

    route add <destination> <gateway> <mask> <interface>

使用例:

.. code-block:: none

    route add 192.168.6.0 192.168.1.100 255.255.255.0 ETH0

関連のソースコード:
:source:`xsh_route() <shell/xsh_route.c>`,
:source:`rtAdd() <network/route/rtAdd.c>`

ルートの削除
--------------

ルートテーブルエントリテーブルからルートを削除するには、次のパラメタと
ともに **route del** を使用します。

.. code-block:: none

    route del <destination>

使用例:

.. code-block:: none

    route del 192.168.6.0

関連のソースコード:
:source:`xsh_route() <shell/xsh_route.c>`,
:source:`rtRemove() <network/route/rtRemove.c>`.

デバッグ
---------

ルーティングサブシステムにはデバッグに使用する :doc:`Trace statements
</development/Trace>` があります。有効にするには :source:`include/route.h`
にある次の行のコメントを外します。オプションとして、ログメッセージを出力
するデバイス（TTY1など）を変更します。

    // #define TRACE_RT TTY1
