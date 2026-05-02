# 5G O-RAN Customization Testbed (OCUDU Fork)

A containerized development environment for researching and modifying L2/L3 layers in a 5G Standalone base station (gNB). 

This fork focuses on streamlining the deployment architecture and introducing software-defined radio emulation for accessible, hardware-free 5G network testing.

## Architecture

The testbed consists of a fully containerized 5G architecture:
- **Core Network:** Open5GS (AMF, UPF, SMF, etc.)
- **Radio Access Network (RAN):** OCUDU gNB (CU/DU)
- **UE Emulation:** srsUE (Connected via ZeroMQ virtual RF)
- **Telemetry:** Telegraf, InfluxDB, Grafana (TIG Stack)

## My Contributions & Enhancements

* **DevOps & Infrastructure:** 
  * Modified the core `Dockerfile` and CMake build scripts to inject native **ZeroMQ (ZMQ)** dependencies. This allows full baseband and RF emulation without requiring expensive physical USRP hardware.
* **Orchestration:** 
  * Designed a unified Bash orchestration script (`manage.sh`) implementing the Entrypoint Pattern to manage the multi-container lifecycle (Core, gNB, UI), handle hard resets, and provide smart logging.
* **Configuration Management:** 
  * Created optimized FDD cell configurations specifically tuned for virtual SDR compatibility between OCUDU and srsUE.

## Quick Start

Deploying the customized 5G network is fully automated via the provided bash wrapper:
```bash
# 1. Start the 5G Core, gNB, and Metrics UI
./manage.sh start-all

# 2. Recompile the C++ source code and restart the radio (Useful during development)
./manage.sh rebuild-gnb

# 3. View smart logs for the gNB
./manage.sh logs gnb
```

## Key Technologies
C++17, Docker / Docker Compose, 5G NR (SA), ZeroMQ, O-RAN, SCTP/NGAP.