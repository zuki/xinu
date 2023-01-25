TFTP
====

:doc:`ネットワークサブシステム <index>` の一部として、**TFTP**
(**Trivial File Transfer Protocol**) のサポートがXINIに追加されました。
現在のところ、クライアントサポートだけが実装されており、その中でも
TFTPのGetリクエストだけがサポートされています（すなわち、ファイルの
ダウンロードはサポートされていますが、ファイルアップロードはサポート
されていません）。APIは :source:`include/tftp.h` で宣言されており、
:source:`tftpGet() <network/tftp/tftpGet.c>` と
:source:`tftpGetIntoBuffer() <network/tftp/tftpGetIntoBuffer.c>` で
実装されています。詳細についてはAPIドキュメントを参照してください。

このページはXINUに組み込まれているTFTPクライアントのサポートだけを説明
したものであり、様々なMIPSベースのルータにあるサードパーティファーム
ウェアである :doc:`CFE </mips/Common-Firmware-Environment>` に含まれて
いるTFTPサポートとは完全に別物であることに注意してください。

また、 :source:`include/tftp.h` にはいくつかのコンパイル時パラメータと
TFTPコードに対して有効にすることができる
:doc:`トレースマクロ </development/Trace>` が含まれています。

参考資料
---------

- :wikipedia:`Trivial File Transfer Protocol - Wikipedia <Trivial File Transfer Protocol>`
- :rfc:`1350`
