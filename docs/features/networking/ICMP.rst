ICMP
====

:doc:`ネットワークサブシステム <index>` の一部として、XINUは **ICMP**
(**Internet Control Message Protocol**) をサポートしています。
実装は :source:`network/icmp/` ディレクトリにあります。このモジュールには
起動時に実行され、ICMPエコー要求（ping）に応答するICMPデーモンが
あります。また、このモジュールには指定されたIPv4アドレスにICMPエコー要求を
送信する **ping** :doc:`shell command </features/Shell>` で使用される
:source:`icmpEchoRequest() <network/icmp/icmpEchoRequest.c>` 関数を実装
しています。

ICMPエコーパケットの送信や応答を行うには（たとえば、シェルで **netup** を
使って）ネットワークインターフェイスを立ち上げる必要があります。

参考資料
---------

- :rfc:`792`
