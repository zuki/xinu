/**
 * @defgroup udp UDP
 * @ingroup devices
 * @brief UDP ドライバ.
 *
 * このモジュールはUD P(User Datagram Protocol) へのインタフェースを提供する。
 *
 * @defgroup udpexternal UDP標準関数
 * @ingroup udp
 * @brief 主なUDP API, Xinuのデバイスモデルに準拠した関数を含む.
 *
 * ユーザはまず、udpAlloc()でUDPデバイスを割り当て、そのopen()、read()、write()、
 * control()、close()といった汎用デバイスコールを使ってここに挙げた対応する関数を
 * 呼び出す必要がある。UDPデバイスは基本的に他のオペレーティングシステムで使用
 * されている「ソケット」に対応している。ローカル/リモートのポート/アドレスは
 * open()コールで指定される。
 *
 * @defgroup udpinternal UDP内部関数
 * @ingroup udp
 * @brief ほとんどがUDPドライバの内部関数である低レベル関数.
 */
