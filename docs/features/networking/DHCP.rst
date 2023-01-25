DHCP
====

:doc:`ネットワークサブシステム <index>` の一部として、**DHCP**
(**Dynamic Host Configuration Protocol**) のサポートがXINUに追加されました。
現在のところ、IPv4クライアント（すなわち、IPv4アドレス情報の取得）だけが
サポートされています。コードは :source:`network/dhcpc` にあり、APIは
:source:`include/dhcpc.h` で宣言されています。

DHCPクライアントは基本的なIPv4情報（割り当てられたIPv4アドレス、
ネットマスク、ゲートウェイ）に加えて、DHCPサーバが提供していれば
"bootfile"オプションと"next-server"オプションを返します。この情報を
指定すると、XINUの :doc:`TFTPクライアント <TFTP>` を使用してブートファイルを
ダウンロードするために使用することができます。

.. note::
    このページはXINUに組み込まれているDHCPクライアントのサポートだけを
    説明したものであり、サードパーティファームウェアであるCFEに含まれて
    いるDHCPサポートとは完全に別物です。

参考資料
---------

- :wikipedia:`Dynamic Host Configuration Protocol - Wikipedia <Dynamic Host Configuration Protocol>`
- :rfc:`2131`
