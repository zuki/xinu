ネットワークブートのためのBBBのセットアップ
============================================

これはほぼ ``uEnv.txt`` というファイルを見つけ、それを修正することを意味します。

最近のDebianベースのシステムでは必要なファイルは ``/uEnv.txt`` であり、
``/boot/uEnv.txt`` では **ありません** 。このファイルは誤解させるために
存在しているだけです。原始的なシステムにはおそらく`/uEnv.txt`という
ファイルは存在 **しません** が、そこに置くことになります。ファイルの
内容は次のようにしてください。

.. code-block:: none

    optargs=quiet
    uenvcmdx=echo Booting via tftp (Xinu); setenv saloadaddr 0x81000000; setenv ipaddr 192.168.0.54; setenv serverip 192.168.0.5; tftpboot ${saloadaddr} xinu.bin; go ${saloadaddr}
    uenvcmd=run uenvcmdx


あるいは次のようにします。

.. code-block:: none

    optargs=quiet
    uenvcmdx=echo Booting via tftp (kyu.bin); setenv saloadaddr 0x80000000; setenv ipaddr 192.168.0.12; setenv serverip 192.168.0.5; tftpboot ${saloadaddr} kyu.bin; go ${saloadaddr}
    uenvcmd=run uenvcmdx

初期のDebianシステムの中には ``uEnv.txt`` を保持するFATパーティションを持つものが
ありました。ただし、ここにはMLOと ``u-boot.img`` はなく、最終的にはこの
パーティションがまったく必要でないことに気付き、廃止されました。しかし、これは
上記が意味をなさないような場合は見るべき場所です。このようなFATパーティションの
利点は、USB経由で「公開」し、（Linuxの起動後は）マウントできることです。この
セットアップには ``/boot/uEnv.txt`` という誤解を招くようなファイルがありました。
このセットアップにはさらに多くの混乱があります。 ``uEnv.txt`` を持つfatパーティションが欲しくても通常はマウントされていません。BBB上で動作するlinuxの中から
目的の ``uEnv.txt`` ファイルにアクセスしたい場合は、次のような方法でパーティションを
マウントする必要があります。

.. code-block:: none

    mount /dev/mmcblk0p1 /mnt

古いAngstromベースのシステムではFATパーティションにMLOと ``uEnv.txt`` がありました
（ ``U-boot.img`` も同様です）。 ``uEnv.txt`` の内容は同じでファイルの場所だけが
変更されています。

ロードアドレス
---------------

これは実行ファイルがどのようにリンクされているかに対応しなければなりません
（最近は通常、"lds" ファイルで指定されています）。私は以前は偏執狂以外の特別な
理由もなくRAMからの開始を避けていました。今では（BBBの ``0x80800000`` にある）
``U-boot`` にRAMの開始点である ``0x80000000`` にロードさせるだけで、問題なく動作して
います。以前は ``0x80300000`` にロードしており、これも問題なく動作しましたが、
RAMの先頭に大きな (3MB) 穴が空いてしまいました。

静的イメージの実行
-------------------

Debianベースのボードでは ``/uEnv.txt`` を次のようにしています。

.. code-block:: none

    uenvcmdx=echo Booting kyu (kyu.bin) from eMMC; setenv saloadaddr 0x80000000; load mmc 1:1 ${saloadaddr} /kyu.bin; go ${saloadaddr}
    uenvcmd=run uenvcmdx

そして ``kyu.bin`` を ``/uEnv.txt`` ファイルとともに ``/kyu.bin`` となるようルートに
配置しました。これで動きました。これで起動にネットワークを必要としない
「すぐに使える」組み込みシステムができあがりました。binファイルはTFTPブートで
使っていたものとまったく同じファイルです。バイトも同じで、同じ場所で終わります。

実行中のLinuxに戻る
---------------------

橋が焼けていないことを確認するのは常に良いことです。そして、時には
メンテナンス（ネットワークブート設定のIP番号の変更など）をする必要があります。

シリアルコンソールが必要です。U-bootが中断するためにキーをタイプするように
指示したら、キーをタイプしてください。そして、U-bootのプロンプトで次のように
タイプします。

.. code-block:: none

    setenv bootenv x.txt
    boot

x.txtは単なる提案に過ぎず、存在しないファイルでも構いません。"linux" や
"abracadabra"など存在しない名前をこの目的に使うことができます。これは
``uEnv.txt`` の実行をブロックし、通常動作をさせるのでLinuxが起動します。
そして、一度だけです。次にリセットしたり電源を入れたりすると、また
ネットワークブートに戻ります。もちろんその間に ``uEnv.txt`` を修正しなければの
話です。

このスキームは存在する代替ファイルの選択にも使え、便利かもしれませんが、
私は使ったことがありません。

トリッキーなケースでの復帰
----------------------------

古いパーティション方式のユニット（パーティション1は、MLOとu-boot.imgを含む
FATパーティションで、パーティション2はLinux）からU-bootイメージを取り出し、
新しいパーティション方式のユニット（パーティション1はLinuxで、それだけ）に
"dd"コマンドを使ってインストールしました。これはネットワークブートは問題なく
動作しますが、Linuxはブート **しない** でしょう。パーティション2のLinuxを
探すのですが存在しないからです。このようなシステムでLinuxを再び起動させるには
SDカードを使い、eMMCパーティションをマウントし、パーティション1にLinuxを探す
U-bootをインストールする必要があります）。

しかし、このような猿芝居は自業自得です。これは、ネットワークブートのみの
暫定措置として、汚いハックだからです。

U-bootをいじる
-----------------

いじりたくなったら、シリアルコンソールを接続しておく必要があります。U-bootには
豊富なコマンドを持つ対話型プロンプトがあります。

「任意のキーを押す」ように促されたら、そうすると、U-bootのプロンプトが表示
されます。ブートシーケンスを再開するには ``"boot"`` と入力します。タイプすべき
便利コマンドは ``"env print"`` です。

``"bootcmd"`` 変数が ``boot`` とタイプしたとき（あるいはU-bootを放置したとき）に
実行される「スクリプト」であることを知っていればなぜそうなるのかを理解する
ことができます。そして、次の出力を研究して、他のアイデアを得ることができます。

"print env"の出力
^^^^^^^^^^^^^^^^^^

.. code-block:: none


    U-Boot# env print
    arch=arm
    baudrate=115200
    board=am335x
    board_name=A335BNLT
    board_rev=000B
    bootcmd=gpio set 53; i2c mw 0x24 1 0x3e; run findfdt; mmc dev 0; if mmc rescan ; then echo micro SD card found;setenv mmcdev 0;else echo No micro SD card found, setting mmcdev to 1;setenv mmcdev 1;fi;setenv bootpart ${mmcdev}:2;mmc dev ${mmcdev}; if mmc rescan; then gpio set 54; echo SD/MMC found on device ${mmcdev};if run loadbootenv; then echo Loaded environment from ${bootenv};run importbootenv;fi;if test -n $uenvcmd; then echo Running uenvcmd ...;run uenvcmd;fi;gpio set 55; if run loaduimage; then gpio set 56; run loadfdt;run mmcboot;fi;fi;
    bootdelay=1
    bootdir=/boot
    bootenv=uEnv.txt
    bootfile=uImage
    bootpart=0:2
    console=ttyO0,115200n8
    cpu=armv7
    dfu_alt_info_emmc=rawemmc mmc 0 3751936
    dfu_alt_info_mmc=boot part 0 1;rootfs part 0 2;MLO fat 0 1;MLO.raw mmc 100 100;u-boot.img.raw mmc 300 3C0;u-boot.img fat 0 1;uEnv.txt fat 0 1
    dfu_alt_info_nand=SPL part 0 1;SPL.backup1 part 0 2;SPL.backup2 part 0 3;SPL.backup3 part 0 4;u-boot part 0 5;kernel part 0 7;rootfs part 0 8
    ethact=cpsw
    ethaddr=1c:ba:8c:9c:e3:01
    fdt_high=0xffffffff
    fdtaddr=0x80F80000
    fdtfile=am335x-boneblack.dtb
    findfdt=if test $board_name = A33515BB; then setenv fdtfile am335x-evm.dtb; fi; if test $board_name = A335X_SK; then setenv fdtfile am335x-evmsk.dtb; fi;if test $board_name = A335BONE; then setenv fdtfile am335x-bone.dtb; fi; if test $board_name = A335BNLT; then setenv fdtfile am335x-boneblack.dtb; fi
    importbootenv=echo Importing environment from mmc ...; env import -t $loadaddr $filesize
    kloadaddr=0x80007fc0
    loadaddr=0x80200000
    loadbootenv=load mmc ${mmcdev} ${loadaddr} ${bootenv}
    loadfdt=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}
    loadramdisk=load mmc ${mmcdev} ${rdaddr} ramdisk.gz
    loaduimage=load mmc ${bootpart} ${kloadaddr} ${bootdir}/${bootfile}
    mmcargs=setenv bootargs console=${console} ${optargs} root=${mmcroot} rootfstype=${mmcrootfstype}
    mmcboot=echo Booting from mmc ...; run mmcargs; bootm ${kloadaddr} - ${fdtaddr}
    mmcdev=0
    mmcroot=/dev/mmcblk0p2 ro
    mmcrootfstype=ext4 rootwait
    mtdids=nand0=omap2-nand.0
    mtdparts=mtdparts=omap2-nand.0:128k(SPL),128k(SPL.backup1),128k(SPL.backup2),128k(SPL.backup3),1920k(u-boot),128k(u-boot-env),5m(kernel),-(rootfs)
    nandargs=setenv bootargs console=${console} ${optargs} root=${nandroot} rootfstype=${nandrootfstype}
    nandboot=echo Booting from nand ...; run nandargs; nand read ${loadaddr} ${nandsrcaddr} ${nandimgsize}; bootm ${loadaddr}
    nandimgsize=0x500000
    nandroot=ubi0:rootfs rw ubi.mtd=7,2048
    nandrootfstype=ubifs rootwait=1
    nandsrcaddr=0x280000
    netargs=setenv bootargs console=${console} ${optargs} root=/dev/nfs nfsroot=${serverip}:${rootpath},${nfsopts} rw ip=dhcp
    netboot=echo Booting from network ...; setenv autoload no; dhcp; tftp ${loadaddr} ${bootfile}; tftp ${fdtaddr} ${fdtfile}; run netargs; bootm ${loadaddr} - ${fdtaddr}
    nfsopts=nolock
    ramargs=setenv bootargs console=${console} ${optargs} root=${ramroot} rootfstype=${ramrootfstype}
    ramboot=echo Booting from ramdisk ...; run ramargs; bootm ${loadaddr} ${rdaddr} ${fdtaddr}
    ramroot=/dev/ram0 rw ramdisk_size=65536 initrd=${rdaddr},64M
    ramrootfstype=ext2
    rdaddr=0x81000000
    rootpath=/export/rootfs
    soc=am33xx
    spiargs=setenv bootargs console=${console} ${optargs} root=${spiroot} rootfstype=${spirootfstype}
    spiboot=echo Booting from spi ...; run spiargs; sf probe ${spibusno}:0; sf read ${loadaddr} ${spisrcaddr} ${spiimgsize}; bootm ${loadaddr}
    spibusno=0
    spiimgsize=0x362000
    spiroot=/dev/mtdblock4 rw
    spirootfstype=jffs2
    spisrcaddr=0xe0000
    static_ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}::off
    stderr=serial
    stdin=serial
    stdout=serial
    usbnet_devaddr=1c:ba:8c:9c:e3:01
    vendor=ti
    ver=U-Boot 2013.04-dirty (Jul 10 2013 - 14:02:53)

    Environment size: 3877/131068 bytes
