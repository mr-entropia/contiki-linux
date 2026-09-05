#!/bin/bash
# test-contiki.sh - run Contiki and the RFB test client inside one
# unprivileged user network namespace.
set -e
cd "$(dirname "$0")"

unshare -Urn bash -c '
set -e
IFACE=contiki0
ip tuntap add dev "$IFACE" mode tap
ip link set "$IFACE" up
ip addr add 172.16.0.1/24 dev "$IFACE"
echo "TAP up"
./contiki "$IFACE" >contiki.log 2>&1 &
CPID=$!
sleep 3
echo "=== RRFB TEST ==="
python3 rfb-test.py 172.16.0.2 5900
RC=$?
kill $CPID 2>/dev/null || true
wait $CPID 2>/dev/null || true
echo "=== CONTIKI LOG ==="
cat contiki.log
exit $RC
'