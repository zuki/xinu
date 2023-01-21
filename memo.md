# ドキュメントの作成

## 1, オリジナル

```
$ cd docs
$ make html
sphinx-build -b html -d _build/doctrees   . _build/html
Sphinx v2.4.4 を実行中

Extension error:
拡張 sphinx.ext.pngmath をimportできません (exception: No module named 'sphinx.ext.pngmath')
make: *** [html] Error 2
```

## 2. conf.pyを修正

[pngmathはremoved](https://github.com/sphinx-doc/sphinx/issues/6182)

```diff
$ git diff conf.py
diff --git a/docs/conf.py b/docs/conf.py
index 27dd145..ae35d1a 100644
--- a/docs/conf.py
+++ b/docs/conf.py
@@ -25,7 +25,8 @@ sys.path.insert(0, os.path.abspath('.'))

 # Add any Sphinx extension module names here, as strings. They can be extensions
 # coming with Sphinx (named 'sphinx.ext.*') or your custom ones.
-extensions = ['sphinx.ext.pngmath', 'sphinx.ext.mathjax', 'xinusource']
+# extensions = ['sphinx.ext.pngmath', 'sphinx.ext.mathjax', 'xinusource']
+extensions = ['sphinx.ext.imgmath', 'sphinx.ext.mathjax', 'xinusource']

 # Add any paths that contain templates here, relative to this directory.
 templates_path = ['_templates']
```

```
make html
...
静的ファイルをコピー中... ... 完了
copying extra files... 完了
dumping search index in English (code: en)... 完了
dumping object inventory... 完了
build 成功, 24 warnings.

HTMLページは_build/htmlにあります。

Build finished. The HTML pages are in _build/html.
```

# arm-rpiをmake

## 1. オリジナル

```
$ make -C compile PLATFORM=arm-rpi
-e 	Building config/config
y.tab.c:1129:16: error: implicit declaration of function 'yylex' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
      yychar = yylex ();
               ^
parse.y:45:25: error: implicit declaration of function 'mktype' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                      { mktype((yyvsp[0].intval)); }
                        ^
parse.y:48:56: error: implicit declaration of function 'cktname' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                         {(yyval.intval) = currtname = cktname((yyvsp[-1].intval)); }
                                                       ^
parse.y:51:53: error: implicit declaration of function 'lookup' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                      { (yyval.intval) = currname = lookup((yyvsp[0].strval)); }
                                                    ^
parse.y:58:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(CSR, (yyvsp[0].intval));     }
                                 ^
parse.y:59:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(IRQ, (yyvsp[0].intval));     }
                                 ^
parse.y:60:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(INTR, (yyvsp[0].intval));    }
                                 ^
parse.y:61:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(OPEN, (yyvsp[0].intval));    }
                                 ^
parse.y:62:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(CLOSE, (yyvsp[0].intval));   }
                                 ^
parse.y:63:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(INIT, (yyvsp[0].intval));    }
                                 ^
parse.y:64:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(GETC, (yyvsp[0].intval));    }
                                 ^
parse.y:65:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(PUTC, (yyvsp[0].intval));    }
                                 ^
parse.y:66:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(READ, (yyvsp[0].intval));    }
                                 ^
parse.y:67:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(WRITE, (yyvsp[0].intval));   }
                                 ^
parse.y:68:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(SEEK, (yyvsp[0].intval));    }
                                 ^
parse.y:69:34: error: implicit declaration of function 'newattr' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                               { newattr(CONTROL, (yyvsp[0].intval)); }
                                 ^
parse.y:79:43: error: implicit declaration of function 'mkdev' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                                        { mkdev((yyvsp[-3].intval), (yyvsp[-1].intval), (yyvsp[0].intval)); }
                                          ^
parse.y:82:51: error: implicit declaration of function 'ckdname' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
                   { (yyval.intval) = currdname = ckdname((yyvsp[0].intval)); }
                                                  ^
y.tab.c:1384:7: error: implicit declaration of function 'yyerror' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
      yyerror (YY_("syntax error"));
      ^
fatal error: too many errors emitted, stopping now [-ferror-limit=]
20 errors generated.
make[1]: *** [parse.o] Error 1
make: *** [config/config] Error 2
```

## 2. `compile/config/parse.y`を修正

```diff
$ git diff compile/config/parse.y
diff --git a/compile/config/parse.y b/compile/config/parse.y
index 964ba89..dae12e8 100644
--- a/compile/config/parse.y
+++ b/compile/config/parse.y
@@ -3,6 +3,17 @@ extern char *doing;
 extern int   currname;
 extern int   currtname;
 extern int   currdname;
+int yylex();
+void yyerror(const char *s);
+
+int  lookup(const char *str);
+void newattr(int tok, int val);
+int  cktname(int symid);
+void mktype(int deviceid);
+void initattr(struct dev_ent *fstr, int tnum, int deviceid);
+void mkdev(int nameid, int typid, int deviceid);
+int  ckdname(int devid);
+
 %}

 /* Token semantic values */
```

```
$ make -C compile PLATFORM=arm-rpi
-e 	Building config/config
parse.y:13:22: warning: declaration of 'struct dev_ent' will not be visible outside of this function [-Wvisibility]
void initattr(struct dev_ent *fstr, int tnum, int deviceid);
                     ^
1 warning generated.
ld: library not found for -lgcc
clang: error: linker command failed with exit code 1 (use -v to see invocation)
make[1]: *** [config] Error 1
make: *** [config/config] Error 2
```

## 3. `compile/config`ディレクトリでmake

```
$ cd compile/config
$ make clean
rm -f config *.o y.tab.h parse.c
dspace@mini:~/raspi_os/xinu/compile/config$ make
bison -y     -d parse.y
mv -f y.tab.c parse.c
gcc    -c -o parse.o parse.c
parse.y:13:22: warning: declaration of 'struct dev_ent' will not be visible outside of this function [-Wvisibility]
void initattr(struct dev_ent *fstr, int tnum, int deviceid);
                     ^
1 warning generated.
gcc    -c -o config.o config.c
flex  -t scan.l > scan.c
gcc    -c -o scan.o scan.c
gcc   config.o scan.o parse.o   -o config
rm scan.c
```

```
$ cd ../..
$ make -C compile PLATFORM=arm-rpi
-e 	Configuring ../system/conf.c
-e 	Compiling ../system/conf.o
-e 	Assembling ../loader/platforms/arm-rpi/start.o
-e 	Assembling ../system/platforms/arm-rpi/ctxsw.o
../system/arch/arm/ctxsw.S: Assembler messages:
../system/arch/arm/ctxsw.S:47: Warning: if writeback register is in list, it must be the lowest reg in the list
-e 	Assembling ../system/platforms/arm-rpi/halt.o
-e 	Assembling ../system/platforms/arm-rpi/intutils.o
-e 	Assembling ../system/platforms/arm-rpi/irq_handler.o
-e 	Assembling ../system/platforms/arm-rpi/memory_barrier.o
-e 	Assembling ../system/platforms/arm-rpi/pause.o
-e 	Compiling ../system/platforms/arm-rpi/setupStack.o
-e 	Compiling ../system/platforms/arm-rpi/bcm2835_power.o
-e 	Compiling ../system/platforms/arm-rpi/dispatch.o
-e 	Compiling ../system/platforms/arm-rpi/kexec.o
-e 	Compiling ../system/platforms/arm-rpi/platforminit.o
-e 	Compiling ../system/platforms/arm-rpi/timer.o
-e 	Compiling ../system/platforms/arm-rpi/usb_dwc_hcd.o
-e 	Compiling ../system/platforms/arm-rpi/watchdog.o
-e 	Compiling ../system/initialize.o
-e 	Compiling ../system/queue.o
-e 	Compiling ../system/create.o
-e 	Compiling ../system/kill.o
-e 	Compiling ../system/ready.o
-e 	Compiling ../system/resched.o
-e 	Compiling ../system/resume.o
-e 	Compiling ../system/suspend.o
-e 	Compiling ../system/chprio.o
-e 	Compiling ../system/getprio.o
-e 	Compiling ../system/getitem.o
-e 	Compiling ../system/queinit.o
-e 	Compiling ../system/insert.o
-e 	Compiling ../system/gettid.o
-e 	Compiling ../system/xdone.o
-e 	Compiling ../system/yield.o
-e 	Compiling ../system/userret.o
-e 	Compiling ../system/clkinit.o
-e 	Compiling ../system/clkhandler.o
-e 	Compiling ../system/mdelay.o
-e 	Compiling ../system/udelay.o
-e 	Compiling ../system/insertd.o
-e 	Compiling ../system/sleep.o
-e 	Compiling ../system/unsleep.o
-e 	Compiling ../system/wakeup.o
-e 	Compiling ../system/semcreate.o
-e 	Compiling ../system/semfree.o
-e 	Compiling ../system/semcount.o
-e 	Compiling ../system/signal.o
-e 	Compiling ../system/signaln.o
-e 	Compiling ../system/wait.o
-e 	Compiling ../system/moncreate.o
-e 	Compiling ../system/monfree.o
-e 	Compiling ../system/moncount.o
-e 	Compiling ../system/lock.o
-e 	Compiling ../system/unlock.o
-e 	Compiling ../system/memget.o
-e 	Compiling ../system/memfree.o
-e 	Compiling ../system/stkget.o
-e 	Compiling ../system/bfpalloc.o
-e 	Compiling ../system/bfpfree.o
-e 	Compiling ../system/bufget.o
-e 	Compiling ../system/buffree.o
-e 	Compiling ../system/send.o
-e 	Compiling ../system/receive.o
-e 	Compiling ../system/recvclr.o
-e 	Compiling ../system/recvtime.o
-e 	Compiling ../system/close.o
-e 	Compiling ../system/control.o
-e 	Compiling ../system/getc.o
-e 	Compiling ../system/open.o
-e 	Compiling ../system/ioerr.o
-e 	Compiling ../system/ionull.o
-e 	Compiling ../system/read.o
-e 	Compiling ../system/putc.o
-e 	Compiling ../system/seek.o
-e 	Compiling ../system/write.o
-e 	Compiling ../system/getdev.o
-e 	Compiling ../system/debug.o
-e 	Compiling ../system/minijava.o
-e 	Compiling ../system/tar.o
-e 	Compiling ../device/ethloop/ethloopClose.o
-e 	Compiling ../device/ethloop/ethloopControl.o
-e 	Compiling ../device/ethloop/ethloopOpen.o
-e 	Compiling ../device/ethloop/ethloopWrite.o
-e 	Compiling ../device/ethloop/ethloopRead.o
-e 	Compiling ../device/ethloop/ethloopInit.o
-e 	Compiling ../device/raw/rawClose.o
-e 	Compiling ../device/raw/rawControl.o
-e 	Compiling ../device/raw/rawDemux.o
-e 	Compiling ../device/raw/rawInit.o
-e 	Compiling ../device/raw/rawOpen.o
-e 	Compiling ../device/raw/rawRead.o
-e 	Compiling ../device/raw/rawRecv.o
-e 	Compiling ../device/raw/rawSend.o
-e 	Compiling ../device/raw/rawWrite.o
-e 	Compiling ../device/smsc9512/colon2mac.o
-e 	Compiling ../device/smsc9512/etherClose.o
-e 	Compiling ../device/smsc9512/etherControl.o
-e 	Compiling ../device/smsc9512/etherInit.o
-e 	Compiling ../device/smsc9512/etherInterrupt.o
-e 	Compiling ../device/smsc9512/etherOpen.o
-e 	Compiling ../device/smsc9512/etherRead.o
-e 	Compiling ../device/smsc9512/etherStat.o
-e 	Compiling ../device/smsc9512/etherWrite.o
-e 	Compiling ../device/smsc9512/smsc9512.o
-e 	Compiling ../device/smsc9512/vlanStat.o
-e 	Compiling ../device/tcp/tcpAlloc.o
-e 	Compiling ../device/tcp/tcpChksum.o
-e 	Compiling ../device/tcp/tcpClose.o
-e 	Compiling ../device/tcp/tcpControl.o
-e 	Compiling ../device/tcp/tcpDemux.o
-e 	Compiling ../device/tcp/tcpFree.o
-e 	Compiling ../device/tcp/tcpGetc.o
-e 	Compiling ../device/tcp/tcpInit.o
-e 	Compiling ../device/tcp/tcpOpen.o
-e 	Compiling ../device/tcp/tcpOpenActive.o
-e 	Compiling ../device/tcp/tcpPutc.o
-e 	Compiling ../device/tcp/tcpRead.o
-e 	Compiling ../device/tcp/tcpRecvAck.o
-e 	Compiling ../device/tcp/tcpRecv.o
-e 	Compiling ../device/tcp/tcpRecvData.o
-e 	Compiling ../device/tcp/tcpRecvListen.o
-e 	Compiling ../device/tcp/tcpRecvOpts.o
-e 	Compiling ../device/tcp/tcpRecvOther.o
-e 	Compiling ../device/tcp/tcpRecvRtt.o
-e 	Compiling ../device/tcp/tcpRecvSynsent.o
-e 	Compiling ../device/tcp/tcpRecvValid.o
../device/tcp/tcpRecvValid.c: In function 'tcpRecvValid':
../device/tcp/tcpRecvValid.c:91:12: warning: 'result' may be used uninitialized in this function [-Wmaybe-uninitialized]
   91 |     return result;
      |            ^~~~~~
-e 	Compiling ../device/tcp/tcpSendAck.o
-e 	Compiling ../device/tcp/tcpSend.o
../device/tcp/tcpSend.c: In function 'tcpSend':
../device/tcp/tcpSend.c:34:12: warning: variable 'window' set but not used [-Wunused-but-set-variable]
   34 |     ushort window = 0;
      |            ^~~~~~
-e 	Compiling ../device/tcp/tcpSendData.o
-e 	Compiling ../device/tcp/tcpSendPersist.o
-e 	Compiling ../device/tcp/tcpSendRst.o
-e 	Compiling ../device/tcp/tcpSendRxt.o
-e 	Compiling ../device/tcp/tcpSendSyn.o
-e 	Compiling ../device/tcp/tcpSendWindow.o
-e 	Compiling ../device/tcp/tcpSeqdiff.o
-e 	Compiling ../device/tcp/tcpSetup.o
-e 	Compiling ../device/tcp/tcpStat.o
-e 	Compiling ../device/tcp/tcpTimer.o
-e 	Compiling ../device/tcp/tcpTimerPurge.o
-e 	Compiling ../device/tcp/tcpTimerRemain.o
-e 	Compiling ../device/tcp/tcpTimerSched.o
-e 	Compiling ../device/tcp/tcpTimerTrigger.o
-e 	Compiling ../device/tcp/tcpWrite.o
-e 	Compiling ../device/telnet/telnetAlloc.o
-e 	Compiling ../device/telnet/telnetClose.o
-e 	Compiling ../device/telnet/telnetControl.o
-e 	Compiling ../device/telnet/telnetFlush.o
-e 	Compiling ../device/telnet/telnetGetc.o
-e 	Compiling ../device/telnet/telnetInit.o
-e 	Compiling ../device/telnet/telnetOpen.o
-e 	Compiling ../device/telnet/telnetPutc.o
-e 	Compiling ../device/telnet/telnetRead.o
../device/telnet/telnetRead.c: In function 'telnetRead':
../device/telnet/telnetRead.c:118:17: warning: this 'if' clause does not guard... [-Wmisleading-indentation]
  118 |                 if (tntptr->flags & TELNET_FLAG_ECHO);
      |                 ^~
../device/telnet/telnetRead.c:119:17: note: ...this statement, but the latter is misleadingly indented as if it were guarded by the 'if'
  119 |                 {
      |                 ^
../device/telnet/telnetRead.c:131:21: warning: this 'if' clause does not guard... [-Wmisleading-indentation]
  131 |                     if (tntptr->flags & TELNET_FLAG_ECHO);
      |                     ^~
../device/telnet/telnetRead.c:132:21: note: ...this statement, but the latter is misleadingly indented as if it were guarded by the 'if'
  132 |                     {
      |                     ^
../device/telnet/telnetRead.c:145:17: warning: this 'if' clause does not guard... [-Wmisleading-indentation]
  145 |                 if (tntptr->flags & TELNET_FLAG_ECHO);
      |                 ^~
../device/telnet/telnetRead.c:146:17: note: ...this statement, but the latter is misleadingly indented as if it were guarded by the 'if'
  146 |                 {
      |                 ^
../device/telnet/telnetRead.c:330:17: warning: this 'if' clause does not guard... [-Wmisleading-indentation]
  330 |                 if (tntptr->flags & TELNET_FLAG_ECHO);
      |                 ^~
../device/telnet/telnetRead.c:331:17: note: ...this statement, but the latter is misleadingly indented as if it were guarded by the 'if'
  331 |                 {
      |                 ^
-e 	Compiling ../device/telnet/telnetServer.o
-e 	Compiling ../device/telnet/telnetWrite.o
-e 	Compiling ../device/uart-pl011/kgetc.o
-e 	Compiling ../device/uart-pl011/kputc.o
-e 	Compiling ../device/uart-pl011/../uart/uartControl.o
-e 	Compiling ../device/uart-pl011/../uart/uartGetc.o
-e 	Compiling ../device/uart-pl011/uartHwInit.o
-e 	Compiling ../device/uart-pl011/uartHwPutc.o
-e 	Compiling ../device/uart-pl011/uartHwStat.o
-e 	Compiling ../device/uart-pl011/../uart/uartInit.o
-e 	Compiling ../device/uart-pl011/uartInterrupt.o
-e 	Compiling ../device/uart-pl011/../uart/uartPutc.o
-e 	Compiling ../device/uart-pl011/../uart/uartRead.o
-e 	Compiling ../device/uart-pl011/../uart/uartWrite.o
-e 	Compiling ../device/uart-pl011/../uart/uartStat.o
-e 	Compiling ../device/uart-pl011/../uart/kprintf.o
-e 	Compiling ../device/uart-pl011/../uart/kvprintf.o
-e 	Compiling ../device/framebuffer_rpi/screenInit.o
-e 	Compiling ../device/framebuffer_rpi/mailbox.o
-e 	Compiling ../device/framebuffer_rpi/drawChar.o
-e 	Compiling ../device/framebuffer_rpi/drawShapes.o
-e 	Compiling ../device/framebuffer_rpi/fbPutc.o
-e 	Compiling ../device/framebuffer_rpi/fbWrite.o
-e 	Compiling ../device/framebuffer_rpi/fbprintf.o
-e 	Compiling ../device/framebuffer_rpi/trig.o
-e 	Compiling ../device/framebuffer_rpi/font.o
-e 	Compiling ../device/udp/udpAlloc.o
-e 	Compiling ../device/udp/udpChksum.o
-e 	Compiling ../device/udp/udpClose.o
-e 	Compiling ../device/udp/udpControl.o
-e 	Compiling ../device/udp/udpDemux.o
-e 	Compiling ../device/udp/udpFreebuf.o
-e 	Compiling ../device/udp/udpGetbuf.o
-e 	Compiling ../device/udp/udpInit.o
-e 	Compiling ../device/udp/udpOpen.o
-e 	Compiling ../device/udp/udpRead.o
-e 	Compiling ../device/udp/udpRecv.o
-e 	Compiling ../device/udp/udpSend.o
-e 	Compiling ../device/udp/udpWrite.o
-e 	Compiling ../device/usb/usbcore.o
-e 	Compiling ../device/usb/usbhub.o
-e 	Compiling ../device/usb/usbdebug.o
../device/usb/usbdebug.c: In function 'usb_get_ascii_string':
../device/usb/usbdebug.c:373:30: warning: taking address of packed member of 'struct usb_string_descriptor' may result in an unaligned pointer value [-Waddress-of-packed-member]
  373 |     utf16le_to_ascii(buf.desc.bString, num_chars, strbuf);
      |                      ~~~~~~~~^~~~~~~~
-e 	Compiling ../device/usbkbd/usbKbdBindDevice.o
-e 	Compiling ../device/usbkbd/usbKbdControl.o
-e 	Compiling ../device/usbkbd/usbKbdGetc.o
-e 	Compiling ../device/usbkbd/usbKbdInit.o
-e 	Compiling ../device/usbkbd/usbKbdInterrupt.o
-e 	Compiling ../device/usbkbd/usbKbdRead.o
-e 	Compiling ../device/usbkbd/usbKbdUnbindDevice.o
-e 	Compiling ../device/loopback/loopbackGetc.o
-e 	Compiling ../device/loopback/loopbackClose.o
-e 	Compiling ../device/loopback/loopbackOpen.o
-e 	Compiling ../device/loopback/loopbackPutc.o
-e 	Compiling ../device/loopback/loopbackWrite.o
-e 	Compiling ../device/loopback/loopbackRead.o
-e 	Compiling ../device/loopback/loopbackControl.o
-e 	Compiling ../device/loopback/loopbackInit.o
-e 	Compiling ../device/tty/ttyControl.o
-e 	Compiling ../device/tty/ttyOpen.o
-e 	Compiling ../device/tty/ttyRead.o
-e 	Compiling ../device/tty/ttyClose.o
-e 	Compiling ../device/tty/ttyGetc.o
-e 	Compiling ../device/tty/ttyInit.o
-e 	Compiling ../device/tty/ttyPutc.o
-e 	Compiling ../device/tty/ttyWrite.o
-e 	Compiling ../apps/date.o
-e 	Compiling ../apps/rdate.o
-e 	Compiling ../apps/timeserver.o
-e 	Compiling ../mailbox/mailboxAlloc.o
-e 	Compiling ../mailbox/mailboxCount.o
-e 	Compiling ../mailbox/mailboxFree.o
-e 	Compiling ../mailbox/mailboxInit.o
-e 	Compiling ../mailbox/mailboxReceive.o
-e 	Compiling ../mailbox/mailboxSend.o
-e 	Compiling ../network/arp/arpAlloc.o
-e 	Compiling ../network/arp/arpDaemon.o
-e 	Compiling ../network/arp/arpGetEntry.o
-e 	Compiling ../network/arp/arpFree.o
-e 	Compiling ../network/arp/arpInit.o
-e 	Compiling ../network/arp/arpLookup.o
-e 	Compiling ../network/arp/arpNotify.o
-e 	Compiling ../network/arp/arpRecv.o
-e 	Compiling ../network/arp/arpSendReply.o
-e 	Compiling ../network/arp/arpSendRqst.o
-e 	Compiling ../network/dhcpc/dhcpClient.o
-e 	Compiling ../network/dhcpc/dhcpRecvReply.o
-e 	Compiling ../network/dhcpc/dhcpSendRequest.o
-e 	Compiling ../network/emulate/emuCorrupt.o
-e 	Compiling ../network/emulate/emuDelay.o
-e 	Compiling ../network/emulate/emuDrop.o
-e 	Compiling ../network/emulate/emuDuplicate.o
-e 	Compiling ../network/emulate/emuReorder.o
-e 	Compiling ../network/emulate/netemu.o
-e 	Compiling ../network/icmp/icmpDestUnreach.o
-e 	Compiling ../network/icmp/icmpEchoReply.o
-e 	Compiling ../network/icmp/icmpEchoRequest.o
-e 	Compiling ../network/icmp/icmpInit.o
-e 	Compiling ../network/icmp/icmpRecv.o
-e 	Compiling ../network/icmp/icmpRedirect.o
-e 	Compiling ../network/icmp/icmpSend.o
-e 	Compiling ../network/icmp/icmpTimeExceeded.o
-e 	Compiling ../network/icmp/icmpDaemon.o
-e 	Compiling ../network/ipv4/dot2ipv4.o
-e 	Compiling ../network/ipv4/ipv4Recv.o
-e 	Compiling ../network/ipv4/ipv4RecvDemux.o
-e 	Compiling ../network/ipv4/ipv4RecvValid.o
-e 	Compiling ../network/ipv4/ipv4Send.o
-e 	Compiling ../network/ipv4/ipv4SendFrag.o
-e 	Compiling ../network/net/netChksum.o
-e 	Compiling ../network/net/netDown.o
-e 	Compiling ../network/net/netFreebuf.o
-e 	Compiling ../network/net/netGetbuf.o
-e 	Compiling ../network/net/netInit.o
-e 	Compiling ../network/net/netLookup.o
-e 	Compiling ../network/net/netRecv.o
-e 	Compiling ../network/net/netSend.o
-e 	Compiling ../network/net/netUp.o
-e 	Compiling ../network/netaddr/netaddrequal.o
-e 	Compiling ../network/netaddr/netaddrhost.o
-e 	Compiling ../network/netaddr/netaddrmask.o
-e 	Compiling ../network/netaddr/netaddrsprintf.o
-e 	Compiling ../network/route/rtAdd.o
-e 	Compiling ../network/route/rtAlloc.o
-e 	Compiling ../network/route/rtClear.o
-e 	Compiling ../network/route/rtDaemon.o
-e 	Compiling ../network/route/rtDefault.o
-e 	Compiling ../network/route/rtInit.o
-e 	Compiling ../network/route/rtLookup.o
-e 	Compiling ../network/route/rtRecv.o
-e 	Compiling ../network/route/rtRemove.o
-e 	Compiling ../network/route/rtSend.o
-e 	Compiling ../network/snoop/snoopCapture.o
-e 	Compiling ../network/snoop/snoopClose.o
-e 	Compiling ../network/snoop/snoopFilter.o
-e 	Compiling ../network/snoop/snoopOpen.o
-e 	Compiling ../network/snoop/snoopPrint.o
-e 	Compiling ../network/snoop/snoopPrintArp.o
-e 	Compiling ../network/snoop/snoopPrintEthernet.o
-e 	Compiling ../network/snoop/snoopPrintIpv4.o
-e 	Compiling ../network/snoop/snoopPrintTcp.o
-e 	Compiling ../network/snoop/snoopPrintUdp.o
-e 	Compiling ../network/snoop/snoopRead.o
-e 	Compiling ../network/tftp/tftpGet.o
-e 	Compiling ../network/tftp/tftpGetIntoBuffer.o
-e 	Compiling ../network/tftp/tftpRecvPackets.o
-e 	Compiling ../network/tftp/tftpSendACK.o
-e 	Compiling ../network/tftp/tftpSendRRQ.o
-e 	Compiling ../shell/shell.o
-e 	Compiling ../shell/lexan.o
-e 	Compiling ../shell/getopt.o
-e 	Compiling ../shell/xsh_clear.o
-e 	Compiling ../shell/xsh_date.o
-e 	Compiling ../shell/xsh_exit.o
-e 	Compiling ../shell/xsh_help.o
-e 	Compiling ../shell/xsh_reset.o
-e 	Compiling ../shell/xsh_sleep.o
-e 	Compiling ../shell/xsh_kill.o
-e 	Compiling ../shell/xsh_ps.o
-e 	Compiling ../shell/xsh_memdump.o
-e 	Compiling ../shell/xsh_memstat.o
-e 	Compiling ../shell/xsh_dumptlb.o
-e 	Compiling ../shell/xsh_user.o
-e 	Compiling ../shell/xsh_uartstat.o
-e 	Compiling ../shell/xsh_gpiostat.o
-e 	Compiling ../shell/xsh_led.o
-e 	Compiling ../shell/xsh_arp.o
-e 	Compiling ../shell/xsh_ethstat.o
-e 	Compiling ../shell/xsh_nc.o
-e 	Compiling ../shell/xsh_netdown.o
-e 	Compiling ../shell/xsh_netemu.o
-e 	Compiling ../shell/xsh_netstat.o
-e 	Compiling ../shell/xsh_netup.o
-e 	Compiling ../shell/xsh_ping.o
-e 	Compiling ../shell/xsh_pktgen.o
-e 	Compiling ../shell/xsh_rdate.o
-e 	Compiling ../shell/xsh_route.o
-e 	Compiling ../shell/xsh_snoop.o
-e 	Compiling ../shell/xsh_tcpstat.o
-e 	Compiling ../shell/xsh_telnet.o
-e 	Compiling ../shell/xsh_telnetserver.o
-e 	Compiling ../shell/xsh_timeserver.o
-e 	Compiling ../shell/xsh_udpstat.o
-e 	Compiling ../shell/xsh_vlanstat.o
-e 	Compiling ../shell/xsh_voip.o
-e 	Compiling ../shell/xsh_xweb.o
-e 	Compiling ../shell/xsh_tar.o
-e 	Compiling ../shell/xsh_turtle.o
-e 	Compiling ../shell/xsh_flashstat.o
-e 	Compiling ../shell/xsh_nvram.o
-e 	Compiling ../shell/xsh_kexec.o
-e 	Compiling ../shell/xsh_usbinfo.o
-e 	Compiling ../shell/xsh_test.o
-e 	Compiling ../shell/xsh_testsuite.o
-e 	Compiling ../test/testhelper.o
-e 	Compiling ../test/test_arp.o
../test/test_arp.c: In function 'test_arp':
../test/test_arp.c:105:10: warning: array subscript 6 is outside array bounds of 'int[1]' [-Warray-bounds]
  105 |     data += sizeof(pcap);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_arp.c:25:12: note: while referencing '_binary_data_testarp_pcap_start'
   25 | extern int _binary_data_testarp_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In file included from ../test/test_arp.c:16:
../test/test_arp.c:180:30: warning: array subscript 13 is outside array bounds of 'int[1]' [-Warray-bounds]
  180 |     failif((0 != memcmp(data + ELOOP_LINKHDRSIZE, arp, pkt->len)), "");
../include/testsuite.h:61:7: note: in definition of macro 'failif'
   61 |  if ( cond ) { testFail(verbose, failmsg); printf("\t%s:%d\r\n", __FILE__, __LINE__); passed = FALSE; } \
      |       ^~~~
../test/test_arp.c:25:12: note: while referencing '_binary_data_testarp_pcap_start'
   25 | extern int _binary_data_testarp_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_arp.c:161:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  161 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_arp.c:25:12: note: while referencing '_binary_data_testarp_pcap_start'
   25 | extern int _binary_data_testarp_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-e 	Compiling ../test/test_mailbox.o
-e 	Compiling ../test/test_semaphore3.o
-e 	Compiling ../test/test_bigargs.o
-e 	Compiling ../test/test_memory.o
-e 	Compiling ../test/test_semaphore4.o
-e 	Compiling ../test/test_bufpool.o
-e 	Compiling ../test/test_messagePass.o
-e 	Compiling ../test/test_semaphore.o
-e 	Compiling ../test/test_deltaQueue.o
-e 	Compiling ../test/test_netaddr.o
-e 	Compiling ../test/test_snoop.o
../test/test_snoop.c: In function 'test_snoop':
../test/test_snoop.c:208:10: warning: array subscript 6 is outside array bounds of 'int[1]' [-Warray-bounds]
  208 |     data += sizeof(pcap);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_snoop.c:23:12: note: while referencing '_binary_data_testsnoop_pcap_start'
   23 | extern int _binary_data_testsnoop_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_snoop.c:210:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  210 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_snoop.c:23:12: note: while referencing '_binary_data_testsnoop_pcap_start'
   23 | extern int _binary_data_testsnoop_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-e 	Compiling ../test/test_ether.o
-e 	Compiling ../test/test_netif.o
../test/test_netif.c: In function 'test_netif':
../test/test_netif.c:142:10: warning: array subscript 6 is outside array bounds of 'int[1]' [-Warray-bounds]
  142 |     data += sizeof(pcap);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_netif.c:25:12: note: while referencing '_binary_data_testnetif_pcap_start'
   25 | extern int _binary_data_testnetif_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_netif.c:144:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  144 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_netif.c:25:12: note: while referencing '_binary_data_testnetif_pcap_start'
   25 | extern int _binary_data_testnetif_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_netif.c:144:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  144 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_netif.c:25:12: note: while referencing '_binary_data_testnetif_pcap_start'
   25 | extern int _binary_data_testnetif_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-e 	Compiling ../test/test_ethloop.o
-e 	Compiling ../test/test_nvram.o
-e 	Compiling ../test/test_system.o
-e 	Compiling ../test/test_ip.o
../test/test_ip.c: In function 'test_ip':
../test/test_ip.c:110:10: warning: array subscript 6 is outside array bounds of 'int[1]' [-Warray-bounds]
  110 |     data += sizeof(pcap);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_ip.c:20:12: note: while referencing '_binary_data_testip_pcap_star'
   20 | extern int _binary_data_testip_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_ip.c:115:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  115 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_ip.c:20:12: note: while referencing '_binary_data_testip_pcap_star'
   20 | extern int _binary_data_testip_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_ip.c:115:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  115 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_ip.c:20:12: note: while referencing '_binary_data_testip_pcap_star'
   20 | extern int _binary_data_testip_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-e 	Compiling ../test/test_preempt.o
-e 	Compiling ../test/test_tlb.o
-e 	Compiling ../test/test_libCtype.o
-e 	Compiling ../test/test_procQueue.o
-e 	Compiling ../test/test_ttydriver.o
-e 	Compiling ../test/test_libLimits.o
-e 	Compiling ../test/test_raw.o
../test/test_raw.c: In function 'test_raw':
../test/test_raw.c:71:10: warning: array subscript 6 is outside array bounds of 'int[1]' [-Warray-bounds]
   71 |     data += sizeof(pcap);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_raw.c:18:12: note: while referencing '_binary_data_testraw_pcap_start'
   18 | extern int _binary_data_testraw_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_raw.c:279:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  279 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_raw.c:18:12: note: while referencing '_binary_data_testraw_pcap_start'
   18 | extern int _binary_data_testraw_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
../test/test_raw.c:279:10: warning: array subscript 10 is outside array bounds of 'int[1]' [-Warray-bounds]
  279 |     data += sizeof(phdr);
      |     ~~~~~^~~~~~~~~~~~~~~
../test/test_raw.c:18:12: note: while referencing '_binary_data_testraw_pcap_start'
   18 | extern int _binary_data_testraw_pcap_start;
      |            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-e 	Compiling ../test/test_udp.o
../test/test_udp.c: In function 'test_udp':
../test/test_udp.c:44:20: warning: variable 'mask' set but not used [-Wunused-but-set-variable]
   44 |     struct netaddr mask;
      |                    ^~~~
../test/test_udp.c:43:20: warning: variable 'dst' set but not used [-Wunused-but-set-variable]
   43 |     struct netaddr dst;
      |                    ^~~
../test/test_udp.c:42:20: warning: variable 'src' set but not used [-Wunused-but-set-variable]
   42 |     struct netaddr src;
      |                    ^~~
-e 	Compiling ../test/test_libStdio.o
-e 	Compiling ../test/test_recursion.o
-e 	Compiling ../test/test_umemory.o
-e 	Compiling ../test/test_libStdlib.o
-e 	Compiling ../test/test_schedule.o
-e 	Compiling ../test/test_libString.o
-e 	Compiling ../test/test_semaphore2.o
-e 	Compiling ../system/main.o
-e 	Object Copy data/testarp.pcap.o
-e 	Object Copy data/testip.pcap.o
-e 	Object Copy data/testnetif.pcap.o
-e 	Object Copy data/testraw.pcap.o
-e 	Object Copy data/testsnoop.pcap.o
-e 	Object Copy data/testudp.pcap.o
-e 	Object Copy data/mytar.tar.o
-e 	Installing ../lib/libxc.a
-e 	Compiling abs.c
-e 	Compiling atoi.c
-e 	Compiling atol.c
-e 	Compiling bzero.c
-e 	Compiling ctype_.c
-e 	Compiling doprnt.c
-e 	Compiling doscan.c
-e 	Compiling fgetc.c
-e 	Compiling fgets.c
-e 	Compiling fprintf.c
-e 	Compiling fputc.c
-e 	Compiling fputs.c
-e 	Compiling fscanf.c
-e 	Compiling labs.c
-e 	Compiling memchr.c
-e 	Compiling memcmp.c
-e 	Compiling memcpy.c
-e 	Compiling memset.c
-e 	Compiling printf.c
-e 	Compiling qsort.c
-e 	Compiling rand.c
-e 	Compiling sprintf.c
-e 	Compiling sscanf.c
-e 	Compiling strchr.c
-e 	Compiling strcmp.c
-e 	Compiling strcpy.c
-e 	Compiling strlcpy.c
-e 	Compiling strlen.c
-e 	Compiling strncat.c
-e 	Compiling strncmp.c
-e 	Compiling strncpy.c
-e 	Compiling strnlen.c
-e 	Compiling strrchr.c
-e 	Compiling strstr.c
-e 	Compiling malloc.c
-e 	Compiling free.c
-e 	Linking xinu.elf
$ ls compile/
Doxyfile    Makefile  config  mkvers.sh  scripts  vn         xinu.elf
Doxymain.c  arch      data    platforms  version  xinu.boot
```
