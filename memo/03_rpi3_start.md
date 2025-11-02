# Raspi 3B+ 起動シーケンス（詳細版）

## _start : loader/platform/arm-rpi3-64/start.S

- core 0だけ実行
- 他のコアはstub内でparkさせられているのでこの関数は実行されない

```bash
	supervisor modeに移行
	reset_handler: 
		例外ベクタを0x0にコピー
		例外ベクタのアドレス (0x0) を VBAR にセット
		bssを0クリア
		各コア用のカーネルスタックをカーネルコードの直後に設定してcore_init_sp[4]にセット
		ヒープメモリに使用する領域の底であるmemheapを設定 = core_init_sp[3]
		FPUを有効にする
		screenInit()を実行: グラフィックサブシステムの初期化
		nulluser()を呼び出す
```

## nulluser() : system/initialize.c

- core 0による初期化の続き

```bash
	platforminit():
		platform変数の設定
		bcm2837_power_init(): 電源初期化, 
		mmu_init(): 恒等マップの作成、ペリフェラル領域とDMA領域はcache不可、
					mmuの開始、tlbr0/trbr1をセット
		mutexテーブルの初期化
		dma_buf_init(): DMAバッファ領域の初期化
		random_init(): ハードウェア乱数の初期化
		mutexの初期化: quetab_mutex, thrtab_nutex[NTHREAD], semtab_mutex[NSEM]
	sysinit(): データ構造とデバイスの初期化
		フリーメモリリストの初期化: memlist, pmblock
		スレッドテーブルの初期化: thrtab[NTHREAD]
		NULLスレッド0-3の初期化:
		セマフォテーブルの初期化: semtab[NSEM]
		モニターテーブルの初期化: montab[NMON]
		バッファプールテーブルの初期化: bfptab[NPOOL]
		スレッドreadyリストの初期化: readylist[NCORES]
		clkinit(): タイマーとsleep queueの初期化: sleepq, 
			   interruptVector[IRQ_TIMER] = clkhandler, enable_irq(IRQ_TIMER)
		mailboxInit(): メールボックステーブルの初期化: mboxtab[NMAILBOX], mboxtabsem
		デバイスの初期化: devtab[NDEVS]
		usbinit(): USBの初期化
			usb_bus_locセマフォの作成
			usb_register_device_driver(&usb_hub_driver): USBハブドライバの登録
			hcd_start(): USBホストコントローラの起動
			usb_alloc_device(NULL): ルートハブの割当
			usb_attach_device(root_hub): ルートハブの接続
				usb_read_device_descriptor(dev, 8): 最大パケットサイズの取得
				usb_set_address(dev, address): デバイスのバスアドレスを設定 (addrはusb_devices[]のインデックス
				usb_read_device_descriptor(): デバイスディスクリプタの取得
				usb_read_configuration_descriptor(dev, 0): 先頭のconfigディスクリプタの取得
				usb_set_configuration(): デバイスの構成（先頭のconfig dscriptorを使用）
				usb_try_to_bind_device_driver(dev): デバイスにドライバをバインド
			usb_root_hub = root_hub: root_hubをUSBサブシステムのルートハブに設定
		netInit(): ネットワークインタフェースの初期化
			ネットワークインタフェーステーブルの初期化: netiftab[NNETIF]
			パケットバッファプールの割当: netpool = bfpalloc();
			arpInit(): ARPの初期化
				ARPテーブルの初期化: arptab[ARP_NENTRY]
				ARP用メールボックスの作成; arpqueue = mailboxAlloc(ARP_NQUEUE)
				ARPデーモンスレッドの生成: ready(create(arpDaemon, ..., CORE_ZERO))
			rtInit(): ルートテーブルの初期化
				ルートテーブルの初期化: rttab[RT_NENTRY]
				ルート用メールボックスの作成: rtqueue = mailboxAlloc(RT_NQUEUE);
				ルートデーモンの生成: ready(create(rtDaemon, ..., CORE_ZERO))
			icmpInit(): ICMPの初期化
				ICMP用メールボックスの作成: icmpqueue = mailboxAlloc(ICMP_NQUEUE)
				echoテーブル（ping用）の初期化: echotab[NPINGQUEUE]
				ICMPデーモンの生成: ready(create(icmpDaemon, ..., CORE_ZERO))
			TCPタイマースレッドの実行
				i = create(tcpTimer), ready(i, RESCHED_NO, CORE_ZERO);
	unparkcore(): セカンダリコアの起動
		セットアップ後に実行する関数(core_nulluser)と引数をセット: corestart[cpuno], init_args[cpuno]
		sev(): コアをunpark
		Core N Mailbox 3 Setレジスタにunaprk後に実行する関数(CoreSetup)のアドレスをセット
			CoreSetup(): SYSTEMモードに切り替え、vectorsをVBARにセット、コアのindexを計算、
						spをセット、mmuを起動(コア毎に起動する必要あり）、corestart()を実行
			core_nulluser(): readylist[cpuid]をresched()することを永久に繰り返す
	enable(): 割り込みの有効化
	ready(create(main, INITSTK, INITPRIO, "MAIN", 0), RESCHED_YES, CORE_ZERO): メインスレッドの起動
	while(1){}: 
```

## CoreSetup() : system/platform/arm-rpi3-64/setupCore.S
