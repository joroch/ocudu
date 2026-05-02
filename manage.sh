#!/bin/bash

# gNB config path
export GNB_CONFIG_PATH="../configs/gnb_zmq_fdd.yml"

# Docker compose paths
COMPOSE_BASE="-f docker/docker-compose.yml"
COMPOSE_UI="-f docker/docker-compose.ui.yml"

case "$1" in
    start-all)
        echo "Starting 5G Core + gNB + UI (TIG)..."
        docker compose $COMPOSE_BASE $COMPOSE_UI up -d
        ;;
    start-core)
        echo "Starting 5G Core..."
        docker compose $COMPOSE_BASE up -d 5gc
        ;;
    stop)
        echo "Stopping and cleaning the network..."
        docker compose $COMPOSE_BASE $COMPOSE_UI down --remove-orphans
        ;;
    rebuild-gnb)
        echo "Recompiling gNB and restarting..."
        docker compose $COMPOSE_BASE up -d --build gnb
        ;;
    logs-gnb)
        docker compose $COMPOSE_BASE logs -f gnb
        ;;
    *)
        echo "Usage: ./manage.sh {start-all|start-core|stop|rebuild-gnb|logs-gnb}"
        exit 1
esac