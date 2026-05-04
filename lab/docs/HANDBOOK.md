#  OCUDU 5G Testbed: Reference Handbook

This document contains the complete command reference, operational procedures, and architectural troubleshooting insights for the OCUDU 5G Standalone virtual testbed.

## 1. Orchestration Commands (`manage.sh`)

`./manage.sh start-all` - Starts 5G Core, gNB, and the UI (Grafana/TIG stack) in the background. 
`./manage.sh start` - Starts only the 5G Core and gNB. 
`./manage.sh start-ue` - Creates the `ue1` network namespace, configures host routing, and starts the virtual `srsUE`. 
`./manage.sh restart` - Restarts the Core and Radio containers (useful to clear ZeroMQ buffers). 
`./manage.sh stop` - Tears down all Docker containers and cleans up the network. 
`./manage.sh rebuild-gnb` - Recompiles the C++ source code of the gNB and restarts the container. 
`./manage.sh logs <service>` - Follows the logs of a specific container (e.g., `gnb`, `5gc`).

## 2. Advanced Traffic & Testing

### Network Routing (Host-to-UE)
*   **Host OS Route:** `sudo ip route add 10.45.0.0/16 via 10.53.1.2` (Tells the Host how to reach the UE subnet via the Core container).
*   **UE Default Route:** `sudo ip netns exec ue1 ip route add default via 10.45.1.1 dev tun_srsue` (Sets the UPF as the default gateway for the UE).
*   *Note: These are automated in `./manage.sh start-ue`.*

### iPerf3 (Throughput Testing)
 Requirement: have iperf3 installed on your machine for client side iperf
1.  **Start Server (Core):** `docker compose -f docker/docker-compose.yml exec -d 5gc iperf3 -s`
2.  **Run Client (UE):** `sudo ip netns exec ue1 iperf3 -c 10.45.1.1 -u -b 10M -t 60`
3.  **Stop Server (Core):** `docker compose -f docker/docker-compose.yml exec 5gc pkill iperf3`
*Note: Server commands are automated in `./manage.sh iperf-start/stop`.*

## 3. Observability & Debugging

### Grafana Dashboards
*   **Access:** `http://localhost:3000` (User: `admin` / Pass: `admin`).
*   Used to monitor RSRP, SNR, and real-time throughput metrics during tests.

### PCAP Extractions & Analysis (Wireshark)

> **Prerequisite:** Run `sudo usermod -aG wireshark $USER` and reboot once to use Wireshark without root.

#### 1. Capturing the Air Interface (MAC & RRC)
The radio logs the MAC/RRC layers internally. To get a clean capture starting from the exact moment of connection:
1. `./manage.sh restart-gnb` *(Clears the radio memory and starts a fresh PCAP)*.
2. `./manage.sh start-ue` *(Attach the mobile)*.
3. Close the UE (`Ctrl+C`) and run `./manage.sh extract-pcaps`.
4. Open the generated file: `wireshark ./pcaps/mac_YYYYMMDD_HHMMSS.pcap`.

**Wireshark Setup for MAC-NR:**
By default, Wireshark may not recognize srsRAN's internal UDP wrapping.
*   **Fix:** Go to *Analyze > Enabled Protocols*, search for `mac-nr`, and ensure `mac-nr-udp` is checked.
*   **Pro-Tip (Filtering):** To isolate a specific session in a noisy PCAP, check your UE terminal for its temporary ID (e.g., `c-rnti=0x4601`) and use the display filter `mac.rnti == 0x4601`.

#### 2. Capturing the Core Network (NGAP)
Instead of relying on internal gNB buffers, capture the SCTP traffic directly from the host machine using `tcpdump`. Run this in a separate terminal before attaching the UE:
```bash
sudo tcpdump -i any sctp -w pcaps/ngap_manual.pcap
```
### Core Network Logs (Open5GS)

To view specific Network Function logs inside the monolithic 5gc container:

    AMF Logs: 
```bash
    docker compose -f docker/docker-compose.yml exec 5gc tail -f /var/log/open5gs/amf.log
```
    UPF Logs: 
```bash
    docker compose -f docker/docker-compose.yml exec 5gc tail -f /var/log/open5gs/upf.log
```
## Architecture Notes & Troubleshooting

The following challenges were addressed during the integration of the virtual L2/L3 SDR layers with the 5G Core:

### ZeroMQ Connectivity & Port Mapping

    Issue: The gNB (Docker) and UE (Host) could not communicate over localhost.

    Resolution: Mapped ZMQ TCP ports across the Docker bridge. The gNB listens on *:2000 and transmits to the host IP 10.53.1.1:2001. The UE is configured inversely.

### srsUE Infinite RACH Loop

    Issue: The UE continuously sent Random Access Transmission preambles without response from the gNB.

    Resolution: Removed time_adv_nsamples = 300 from the UE configuration. This parameter compensates for physical copper wire delays and breaks synchronization in zero-latency virtual ZMQ environments.

### RRC Context Release Timeouts

    Issue: The connection dropped some time after attaching, with the gNB reporting RRC container not ACKed within 120msec.

    Resolution: Increased RRC timers (t300, t301, t310, t319) to 2000ms in the gNB configuration to account for CPU scheduling delays during software emulation. (Note: Later reverted to default as a stable ZMQ connection was achieved without them).

### UE Internet Access

    Issue: The UE attached and received an IP (10.45.1.2), but ping 8.8.8.8 failed due to missing return routes.

    Resolution: Established bidirectional routing by adding a static route on the Host pointing the UE subnet to the UPF Docker container (ip route add 10.45.0.0/16 via 10.53.1.2) and setting the default gateway inside the UE namespace.

### Managing Ghost Connections

    Issue: After closing the UE (Ctrl+C), starting a new UE failed to attach.

    Resolution: The gNB holds the MAC context and ZMQ TCP ports remain in TIME_WAIT. Fast-restarting the radio (./manage.sh restart-gnb or ./manage.sh restart) clears the buffers and allows immediate reconnection without restarting the entire 5G Core.

