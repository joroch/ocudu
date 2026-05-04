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
  * Configured cross-bridge TCP routing between the Dockerized gNB and the host-based UE.
* **Orchestration:** 
  * Designed a unified Bash orchestration script (`manage.sh`) implementing the Entrypoint Pattern to manage the multi-container lifecycle (Core, gNB, UI), handle hard resets, and provide smart logging.
  * The script automates static routing between the Host OS and UPF container, giving the UE instant internet access without manual iptables configuration.
* **Configuration Management:** 
  * Created optimized FDD cell configurations specifically tuned for virtual SDR compatibility between OCUDU and srsUE.

## Prerequisites & Environment

This testbed was built and tested on the following environment:
*   **OS:** Ubuntu 22.04 LTS
*   **Containerization:** Docker Engine & Docker Compose v2
*   **Permissions:** The current user must be added to the `docker` group (`sudo usermod -aG docker $USER`) to run the orchestrator without root.
*   **Network Tools:** `iperf3` must be installed on the host OS for throughput testing (`sudo apt install iperf3`).
*   **Build Essentials (For UE):** described in 1. of the UE section.

## Quick Start

Deploying the customized 5G network is fully automated via the provided bash wrapper:
```bash
# Start the 5G Core, gNB, and Metrics UI
./manage.sh start-all

# View smart logs for the services
./manage.sh logs <service>

## dev:
# Recompile the C++ source code and restart the radio (Useful during development)
./manage.sh rebuild-gnb
```

## End-to-End Testing (Virtual UE)

To test the 5G network without physical RF hardware (like USRPs), you can use the srsUE emulator connected to our gNB via ZeroMQ.

**1. Install & Build srsUE (Host Machine)**

*note:only do this on the first time*
Run these commands outside of Docker to compile the UE emulator with ZMQ support:
```bash
sudo apt update
sudo apt install -y build-essential cmake libfftw3-dev libmbedtls-dev libboost-program-options-dev libconfig++-dev libsctp-dev libzmq3-dev

git clone [https://github.com/srsran/srsRAN_4G.git](https://github.com/srsran/srsRAN_4G.git)
cd srsRAN_4G
mkdir build && cd build
cmake ../ -DENABLE_EXPORT=ON -DENABLE_ZEROMQ=ON
make -j$(nproc)
```

**2. Connect the UE**

Use the orchestrator to automatically create the isolated network namespace (ue1) and launch the UE with high CPU priority:
```bash
# In Terminal 1: Start the Core and Radio
./manage.sh start

# In Terminal 2: Launch the Virtual UE
./manage.sh start-ue #(make sure "UE_EXEC_PATH" is in the correct path)
```
If successful, the UE will attach to the network, complete the RRC handshake, and the UPF will assign a PDU session IP (e.g., 10.45.1.2).

**3. Route Traffic**

You can now route real internet traffic through the 5G network:

```bash
sudo ip netns exec ue1 ping 8.8.8.8
```
For more commands or troubleshooting, check out the HANDBOOK.md file for advanced routing, iPerf3 testing, and PCAP extraction.

## Key Technologies
C++17, Docker / Docker Compose, 5G NR (SA), ZeroMQ, O-RAN, SCTP/NGAP.
