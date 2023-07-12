# USB関連の構造体と関数のまとめ

## データのUSB転送関数

- usb_control_msg()          : 同期コントロール転送
- usb_submit_xfer_request() : 非同期バルク、インターラクティブ転送

## USB関連の構造体

### USB 2.0定義の構造体

- `struct usb_control_setup_data`       : コントロールリクエストの標準SETUP構造体
- `struct usb_device_descriptor`        : デバイスディスクリプタ構造体
- `struct usb_configuration_descriptor` : コンフィグレーションディスクリプタ構造体
- `struct usb_interface_descriptor`     : インタフェースディスクリプタ構造体
- `struct usb_endpoint_descriptor`      : エンドポイントディスクリプタ構造体
- `struct usb_string_descriptor`        : ストリングディスクリプタ構造体
- `struct usb_hub_descriptor`           : ハブディスクリプタ構造体

### USB基本構造体

- `struct usb_device`        : USBデバイス構造体
- `struct usb_hub`           : USBハブ構造体
- `struct usb_port`          : USBポート構造体
- `struct usb_device_driver` : USBデバイスドライバ構造体
- `struct usb_xfer_request`  : USBデータ転送要求構造体

### ステータス構造体

- `struct usb_device_status` : デバイスステータス
- `struct usb_hub_status`    : ハブステータス
- `struct usb_port_status`   : ポートステータス

## USB関数

### USBサブシステム

- usbinit() -> syscall

### USBコアドライバ関数

#### USBデバイスドライバ用

- usb_register_device_driver() -> usb_status_t
- usb_alloc_xfer_request() -> struct usb_xfer_request *
- usb_init_xfer_request() -> void
- usb_free_xfer_request() -> void
- usb_submit_xfer_request() -> usb_status_t
- usb_control_msg() -> usb_status_t
- usb_get_descriptor() -> usb_status_t

#### 文字列取得用

- usb_get_string_descriptor() -> usb_status_t
- usb_get_ascii_string() -> usb_status_t
- usb_device_description() -> const char *
- usb_class_code_to_string() -> const char *
- usb_transfer_type_to_string() -> const char *
- usb_direction_to_string() -> const char *
- usb_speed_to_string() -> const char *

#### USBハブ用

- usb_alloc_device() -> struct usb_device *
- usb_attach_device() -> usb_status_t
- usb_free_device() -> void

#### USBホストコントローラ用

- usb_complete_xfer() -> void
- usb_lock_bus() -> void
- usb_unlock_bus() -> void

### USBホストコントローラ関数

- hcd_start() -> usb_status_t
- hcd_stop() -> hcd_stop
- hcd_submit_xfer_request() -> usb_status_t

### USBハブ関数

- usb_hub_for_device_in_tree() -> void

### LAN7800 EthernetデバイスUSBコールバック関数

- lan7800_tx_complete() -> void
- lan7800_rx_complete() -> void
