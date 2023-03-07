実装実験メモ
=============

DHCPでDNS情報が得られるか
--------------------------

Discovery要求の要求リストに ``DHCP_OPT_DNS`` を追加して、DHCP_TRACEを有効化
    -> DNSサーバのIPアドレスが得られた

.. code-block:: none

    xsh$ netup
    WARNING: defaulting to network device ETH0
    Trying DHCP on ETH0...
    ../network/dhcpc/dhcpClient.c:103 (15) Sending DHCPDISCOVER
    ../network/dhcpc/dhcpClient.c:109 (15) Sent DHCPDISCOVER
    ../network/dhcpc/dhcpClient.c:121 (15) Waiting for DHCPOFFER
    ../network/dhcpc/dhcpRecvReply.c:136 (16) Received packet (len=590).
    ../network/dhcpc/dhcpRecvReply.c:169 (16) Received DHCP reply.
    ../network/dhcpc/dhcpRecvReply.c:229 (16) DHCP_OPT_MSGTYPE: 2
    ../network/dhcpc/dhcpRecvReply.c:207 (16) Reached DHCP_OPT_END.
    ../network/dhcpc/dhcpClient.c:137 (15) Sending DHCPREQUEST
    ../network/dhcpc/dhcpClient.c:143 (15) Sent DHCPREQUEST
    ../network/dhcpc/dhcpClient.c:157 (15) Waiting for DHCPACK
    ../network/dhcpc/dhcpRecvReply.c:136 (17) Received packet (len=590).
    ../network/dhcpc/dhcpRecvReply.c:169 (17) Received DHCP reply.
    ../network/dhcpc/dhcpRecvReply.c:229 (17) DHCP_OPT_MSGTYPE: 5
    ../network/dhcpc/dhcpRecvReply.c:207 (17) Reached DHCP_OPT_END.
    ../network/dhcpc/dhcpRecvReply.c:374 (17) Set ip=192.168.10.106
    ../network/dhcpc/dhcpRecvReply.c:377 (17) Set mask=255.255.255.0
    ../network/dhcpc/dhcpRecvReply.c:382 (17) Set gateway=192.168.10.1
    ../network/dhcpc/dhcpRecvReply.c:391 (17) Set dns=192.168.10.1
    ../network/dhcpc/dhcpRecvReply.c:399 (17) TFTP server=NULL
    ../network/dhcpc/dhcpRecvReply.c:400 (17) Bootfile=
    ../network/dhcpc/dhcpClient.c:163 (15) Received DHCPACK
    ETH0 is 192.168.10.106 with netmask 255.255.255.0 (gateway 192.168.10.1)
    xsh$ netstat
    ETH0:
            HW Addr: B8:27:EB:AB:E8:48
            IP Addr: 192.168.10.106    Mask: 255.255.255.0
            Gateway: 192.168.10.1      Bcast IP: 192.168.10.255
            MTU: 1500                  Link Hdr Len: 14
            Num Rcv: 0                 Num Proc: 0
    xsh$ netdown ETH0
    xsh$ netstat
    xsh$
