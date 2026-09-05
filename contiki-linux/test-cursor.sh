#!/bin/bash
set -e
cd "$(dirname "$0")"
unshare -Urn bash -c '
set -e
ip link set lo up 2>/dev/null || true
ip tuntap add dev contiki0 mode tap 2>/dev/null || true
ip link set contiki0 up
ip addr add 172.16.0.1/24 dev contiki0 2>/dev/null || true
./contiki contiki0 >contiki.log 2>&1 &
CPID=$!
sleep 2
python3 test-cursor-client.py 172.16.0.2 5900
RC=$?
kill $CPID 2>/dev/null || true
exit $RC
'