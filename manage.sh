#!/bin/bash

# ==========================================
# 5G Testbed Orchestrator
# ==========================================

#Paths
export GNB_CONFIG_PATH="../lab/configs/gnb_zmq_fdd.yml"
UE_EXEC_PATH="../srsRAN_4G/build/srsue/src/srsue"
UE_CONF_PATH="lab/configs/ue_zmq.conf"

COMPOSE_BASE="-f docker/docker-compose.yml"
COMPOSE_UI="-f docker/docker-compose.ui.yml"

case "$1" in
    start-all)
        echo "Starting 5G Core + gNB + UI (TIG)..."
        docker compose $COMPOSE_BASE $COMPOSE_UI up -d
        ;;
    start)
        echo "Starting 5G Core + gNB..."
        docker compose $COMPOSE_BASE up -d
        ;;
    start-ue)
        echo "Setting up routing and UE..."
        # Inform the Host PC how to find the 5G UE subnet
        sudo ip route add 10.45.0.0/16 via 10.53.1.2 2>/dev/null
        
        # Create the namespace for the UE
        sudo ip netns add ue1 2>/dev/null
        
        echo "Scheduling automatic route injection (Background)..."
        
        (sleep 6 && sudo ip netns exec ue1 ip route add default via 10.45.1.1 dev tun_srsue 2>/dev/null) &
        
        echo "Launching UE (Foreground)..."
        # Launch UE normally for the trace 't' to keep working
        sudo $UE_EXEC_PATH $UE_CONF_PATH
        ;;
    restart)
        echo "Restarting Radio & Core..."
        docker compose $COMPOSE_BASE restart
        ;;
    stop)
        echo "Stopping and cleaning the network..."
        docker compose $COMPOSE_BASE $COMPOSE_UI down --remove-orphans
        ;;
    rebuild-gnb)
        echo "Recompiling gNB and restarting..."
        docker compose $COMPOSE_BASE up -d --build gnb
        ;;
    iperf-start)
        echo "Starting iPerf server on the 5G core..."
        docker compose $COMPOSE_BASE exec -d 5gc iperf3 -s
        ;;
    iperf-stop)
        echo "Stopping iPerf server..."
        docker compose $COMPOSE_BASE exec 5gc pkill iperf3
        ;;
    extract-pcaps)
        TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
        mkdir -p pcaps
        echo "Extracting PCAPS from the current session..."
        docker compose $COMPOSE_BASE cp gnb:/tmp/gnb_ngap.pcap ./pcaps/ngap_${TIMESTAMP}.pcap
        docker compose $COMPOSE_BASE cp gnb:/tmp/gnb_mac.pcap ./pcaps/mac_${TIMESTAMP}.pcap
        echo "Saved in ./pcaps/"
        ls -l ./pcaps/*${TIMESTAMP}*
        ;;
    logs)
        if [ -z "$2" ]; then
            echo "❌ Error: Specify a service. e.g.: ./manage.sh logs gnb (5gc, mongo...)"
        else
            docker compose $COMPOSE_BASE $COMPOSE_UI logs -f "$2"
        fi
        ;;
    *)
    echo "=========================================="
    echo " ⚙️ Usage: ./manage.sh [COMMAND]"
    echo "=========================================="
    echo " Infrastructure:"
    echo "   start-all     - Start 5G Core, Radio, and UI"
    echo "   start         - Start 5G Core and Radio only"
    echo "   stop          - Stop all containers and clean network"
    echo "   restart       - Restart Radio and Core containers"
    echo "   rebuild-gnb   - Recompile the gNB image with new configs"
    echo ""
    echo " UE & Testing:"
    echo "   start-ue      - Launch virtual srsUE with routing"
    echo "   iperf-start   - Start iPerf3 server on the 5G Core"
    echo "   iperf-stop    - Stop iPerf3 server"
    echo ""
    echo " Diagnostics:"
    echo "   extract-pcaps - Save MAC PCAPs to ./pcaps folder"
    echo "   logs <svc>    - View logs for a service (e.g., gnb, 5gc)"
    exit 1
esac