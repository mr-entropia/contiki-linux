#!/bin/bash
#
# run-contiki.sh - Start the Contiki Linux port with its virtual
# Ethernet interface (TAP) and VNC server.
#
# The Contiki uIP stack speaks raw Ethernet on a TAP interface named
# "contiki0". Creating a TAP device requires either root privileges or
# an unprivileged user network namespace (user namespaces). This
# script falls back to the latter when the former is not available,
# which lets the entire Contiki system (including a local VNC client)
# run as a normal user.
#
# Usage:
#   run-contiki.sh [tap-device-name]             - run Contiki only
#   run-contiki.sh --client <cmd...> [tap-name]  - run Contiki and
#                                                  start a VNC client
#                                                  in the same namespace
#
# Contiki listens for VNC on 172.16.0.2:5900.
#
# Internet access: the unprivileged user network namespace has no route
# out, so a userspace relay is used. inet-agent.py runs on the host
# (which has real connectivity) and inet-gateway.py runs inside the
# namespace and answers DNS and forwards HTTP on behalf of Contiki.
# This is enough for Contiki 1.2's HTTP web browser.
#
# Optional configuration (environment):
#   CONTIKI_TAP_NAME   TAP device name (default contiki0)
#   CONTIKI_SUBNET     subnet prefix (default 172.16.0)

set -e

PORTDIR=$(cd "$(dirname "$0")" && pwd)
BIN="$PORTDIR/contiki"

CLIENT=()
IFACE="contiki0"
while [ $# -gt 0 ]; do
    case "$1" in
        --client) shift; while [ $# -gt 0 ] && [ "$1" != "--tap" ]; do
                      CLIENT+=("$1"); shift
                  done ;;
        --tap) shift; IFACE="$1"; shift ;;
        -*) shift ;;
        *) IFACE="$1"; shift ;;
    esac
done

IP_SERVER="172.16.0.2"
IP_HOST="172.16.0.1"
NETMASK="255.255.255.0"
AGENT="$PORTDIR/inet-agent.py"
GATEWAY="$PORTDIR/inet-gateway.py"

start_gateway() {
    # Host-side Internet relay. It runs in the initial network
    # namespace (which has real connectivity) and speaks to the
    # userns-side gateway over Unix sockets in /tmp.
    if [ -x "$(command -v python3)" ] && [ -f "$AGENT" ]; then
        python3 "$AGENT" >/dev/null 2>&1 &
        AGENT_PID=$!
    else
        AGENT_PID=""
    fi
}

stop_gateway() {
    if [ -n "$AGENT_PID" ]; then
        kill "$AGENT_PID" 2>/dev/null || true
    fi
    rm -f /tmp/contiki-dns-gw.sock \
          /tmp/contiki-dns-agent.sock \
          /tmp/contiki-http.sock 2>/dev/null || true
}

if [ ! -x "$BIN" ]; then
    echo "contiki binary not found - run 'make' in $PORTDIR first" >&2
    exit 1
fi

maybe_we_have_root() {
    [ "$(id -u)" = "0" ] && return 0
    if ip tuntap add dev "$$-probe" mode tap 2>/dev/null; then
        ip tuntap del dev "$$-probe" mode tap 2>/dev/null || true
        return 0
    fi
    return 1
}

setup_iface() {
    ip tuntap add dev "$IFACE" mode tap
    ip link set "$IFACE" up
    ip addr add "$IP_HOST/$NETMASK" dev "$IFACE"
}

run_userns() {
    # Build the inner script as an argument list to avoid quoting issues.
    local inner=(
        "set -e"
        "IFACE='$IFACE'"
        "IP_HOST='$IP_HOST'"
        "IP_SERVER='$IP_SERVER'"
        "NETMASK='$NETMASK'"
        "BIN='$BIN'"
        "GATEWAY='$GATEWAY'"
        "$(set -- "${CLIENT[@]}"; [ $# -gt 0 ] && echo "CLIENT=($(printf '%q ' "$@"))" || echo "CLIENT=()")"
        "setup_iface() {"
        "    ip tuntap add dev \"\$IFACE\" mode tap"
        "    ip link set \"\$IFACE\" up"
        "    ip addr add \"\$IP_HOST/\$NETMASK\" dev \"\$IFACE\""
        "}"
        "setup_iface"
        "echo \"TAP \$IFACE up (host \$IP_HOST, contiki \$IP_SERVER)\" >&2"
        "GW_PID=\"\""
        "if [ -f \"\$GATEWAY\" ]; then"
        "    python3 \"\$GATEWAY\" >/dev/null 2>&1 &"
        "    GW_PID=\$!"
        "fi"
        "cleanup() {"
        "    [ -n \"\$GW_PID\" ] && kill \$GW_PID 2>/dev/null || true"
        "}"
        "trap cleanup EXIT"
        "if [ \${#CLIENT[@]} -gt 0 ]; then"
        "    \"\$BIN\" \"\$IFACE\" &"
        "    CONTIKI_PID=\$!"
        "    sleep 2"
        "    \"\${CLIENT[@]}\""
        "    kill \$CONTIKI_PID 2>/dev/null || true"
        "    wait \$CONTIKI_PID 2>/dev/null || true"
        "else"
        "    exec \"\$BIN\" \"\$IFACE\""
        "fi"
    )
    unshare -Urn bash -c "$(printf '%s\n' "${inner[@]}")"
}

if maybe_we_have_root; then
    echo "Creating TAP interface $IFACE (using privileges)" >&2
    setup_iface_root() {
        ip tuntap add dev "$IFACE" mode tap
        ip link set "$IFACE" up
        ip addr add "$IP_HOST/$NETMASK" dev "$IFACE" || true
    }
    setup_iface_root
    trap 'ip tuntap del dev "'"$IFACE"'" mode tap 2>/dev/null || true' EXIT
    start_gateway
    trap 'stop_gateway; ip tuntap del dev "'"$IFACE"'" mode tap 2>/dev/null || true' EXIT
    echo "Starting Contiki on $IFACE" >&2
    exec "$BIN" "$IFACE"
else
    echo "No privileges for a TAP interface - using an unprivileged" >&2
    echo "user network namespace (unshare -Urn)." >&2
    start_gateway
    stop_gateway() {
        if [ -n "$AGENT_PID" ]; then
            kill "$AGENT_PID" 2>/dev/null || true
        fi
        rm -f /tmp/contiki-dns-gw.sock \
              /tmp/contiki-dns-agent.sock \
              /tmp/contiki-http.sock 2>/dev/null || true
    }
    trap 'stop_gateway' EXIT
    run_userns
fi