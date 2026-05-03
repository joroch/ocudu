#!/bin/bash

# ==========================================
# 5G Testbed Orchestrator
# ==========================================

#Paths
export GNB_CONFIG_PATH="../configs/gnb_zmq_fdd.yml"
UE_EXEC_PATH="../srsRAN_4G/build/srsue/src/srsue"
UE_CONF_PATH="configs/ue_zmq.conf"

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
        
        # Inform the Host PC how to find the 5G UE subnet (via the Core container)
        sudo ip route add 10.45.0.0/16 via 10.53.1.2 2>/dev/null
        
        # Create the namespace for the UE
        sudo ip netns add ue1 2>/dev/null
        
        echo "Launching UE..."
        # Start UE in the background to allow route injection
        sudo $UE_EXEC_PATH $UE_CONF_PATH &
        UE_PID=$!
        
        # Wait for the UE to attach and create the tun_srsue interface
        sleep 5
        echo "Injecting UE default route..."
        sudo ip netns exec ue1 ip route add default via 10.45.1.1 dev tun_srsue 2>/dev/null
        
        # Bring UE back to foreground
        wait $UE_PID
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
    logs)
        if [ -z "$2" ]; then
            echo "❌ Error: Specify a service. e.g.: ./manage.sh logs gnb (5gc, mongo...)"
        else
            docker compose $COMPOSE_BASE $COMPOSE_UI logs -f "$2"
        fi
        ;;
    *)
        echo "⚙️ Usage: ./manage.sh {start|start-all|start-ue|stop|restart|rebuild-gnb|logs <service>}"
        exit 1
esac