/**
 * @file usb_hcdi.h
 * @ingroup usbhcd
 *
 * このファイルでは、USBコアドライバが期待するUSBホストコントローラへの
 * インタフェースを宣言し、ドキュメント化する。USB 2.0仕様ではこのような
 * インタフェースはHCDI (Host Controller Driver Interface)と呼ばれている。
 * この設計の目的は、特定のUSBホストコントローラハードウェアへの依存を
 * 分離することである。これは（おそらく残念なことに）USB 2.0仕様自体では
 * 標準化されていない。
 *
 * HCDIはUSBコアドライバ（USBデバイスドライバではなく）だけに使用される
 * ことを意図している。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_HCDI_H_
#define _USB_HCDI_H_

#include <usb_util.h>

struct usb_xfer_request;

/**
 * @ingroup usbhcd
 *
 * USBホストコントローラの電源投入、リセット、初期化を行い、使用可能な
 * 既知の状態にする. これは hcd_power_on() を呼び出した後で、かつ、他のホスト
 * コントローラドライバインタフェース関数を呼び出す前に呼び出す必要がある。
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCESS; そうでない場合は、::usb_status_t の
 *      エラーコード
 */
usb_status_t hcd_start(void);

/**
 * @ingroup usbhcd
 *
 * hcd_start() の呼び出しが成功した時とは逆の処理を行い、USBホスト
 * コントローラを停止する.  後の段階でUSBの初期化に失敗した場合にのみ
 * 使用することを意図しいる。
 */
void hcd_stop(void);

/**
 * @ingroup usbhcd
 *
 * 転送リクエストをUSBホストコントローラに送信して転送を実行する.
 * usb_submit_xfer_request() のドキュメントにあるように、これは非同期の
 * インタフェースを意図している。また、ホストコントローラドライバは
 * リクエストのインテリジェントなスケジューリングを行う責任がある。
 *
 * @param req
 *      実行するリクエストへのポインタ。ホストコントローラドライバに
 *      渡す前にリクエストに対して行うサニティチェックについては
 *      USBコアドライバの usb_submit_xfer_request() の実装を参照されたい。
 *
 * @return
 *      リクエストのエンキュー（必ずしも実行が化膿したわけではない、
 *      まだ開始もしていないかもしれない）が成功した場合は ::USB_STATUS_SUCCESS;
 *      ホストコントローラドライバまたはホストコントローラハードウェアが
 *      指定されたリクエストのタイプまたはスピードをサポートしていない場合は
 *      ::USB_STATUS_INVALID_PARAMETER
 */
usb_status_t hcd_submit_xfer_request(struct usb_xfer_request *req);

#endif /* _USB_HCDI_H_ */
