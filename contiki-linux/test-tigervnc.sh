#!/bin/bash
set -e
cd "$(dirname "$0")"
unshare -Urn bash -c '
set -e
IFACE=contiki0
ip tuntap add dev "$IFACE" mode tap
ip link set "$IFACE" up
ip addr add 172.16.0.1/24 dev "$IFACE"
./contiki "$IFACE" >contiki.log 2>&1 &
CPID=$!
sleep 2
python3 test-tigervnc-client.py 172.16.0.2 5900
RC=$?
kill $CPID 2>/dev/null || true
wait $CPID 2>/dev/null || true
echo "=== CONTIKI LOG ==="
cat contiki.log
exit $RC
'