#!/bin/bash
# End-to-end Internet gateway test: agent in the host netns, gateway in
# an unprivileged userns, then DNS + HTTP probes from inside.
set -e
cd "$(dirname "$0")"
python3 inet-agent.py >inet-agent.log 2>&1 &
APID=$!
trap 'kill $APID 2>/dev/null || true' EXIT
sleep 1
unshare -Urn bash -c '
set -e
IFACE=contiki0
ip link set lo up 2>/dev/null || true
ip tuntap add dev "$IFACE" mode tap
ip link set "$IFACE" up
ip addr add 172.16.0.1/24 dev "$IFACE"
python3 inet-gateway.py >gw.log 2>&1 &
GPID=$!
sleep 1
python3 inet-test.py 172.16.0.1
RC=$?
kill $GPID 2>/dev/null || true
exit $RC
'
echo "=== inet-agent.log ==="; cat inet-agent.log 2>/dev/null | tail -5
echo "=== gw.log ==="; cat gw.log 2>/dev/null | tail -5