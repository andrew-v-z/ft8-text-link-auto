# FT8-Based Automated Parallel Tactical HF Modem (`ft8-text-link-auto`)

An advanced, high-efficiency, and highly automated tactical High Frequency (HF) data communication network protocol. This system leverages the FT8 protocol's extreme weak-signal processing engine, scaled up into a hybrid TDMA/FDMA parallel channel architecture.

This software transforms a standard HF/VHF transceiver into an automated text messenger capable of reliable data exchange under heavy noise and ionospheric fading, requiring no operator intervention for link establishment, clock correction, or network routing.

---

## ⚡ Core Concepts & Theoretical Foundation

* **Deep Noise Penetration:** The core modem utilizes the underlying mathematical engine of FT8, allowing error-free data decoding at Signal-to-Noise Ratios (SNR) down to **-20 dB** — conditions where human voice or standard digital modes fail completely.
* **Audio-Bandwidth Multiplexing:** All data processing, modulation, and demodulation take place within a standard 3 kHz Single Sideband (SSB) voice bandwidth, making it compatible with any conventional analog radio transceiver.
* **Burst Packetized Transmission:** Data is encapsulated into tight, highly optimized burst packets.
* **Parallel Capacity (Multi-Channel Scaling):** Standard FT8 transmits 12–13 characters (77 bits) per 15-second block. This system implements frequency-domain multiplexing to parallelize payloads, allowing a single message to carry up to **384 characters (composed of 32 aggregated 12-character blocks)** within a compressed time-slot window.
* **The Shannon Limit Trade-off:** In strict accordance with Shannon's Channel Capacity Theorem, increasing the text payload requires more subcarriers, which splits the transceiver's total output power. Hence, a 24-character message yields maximum "punching power" at -20 dB, while a maximum 384-character payload requires a relatively cleaner channel.
* **Strict Time-Slot Synchronization:** The protocol relies on precise time synchronization between nodes (error margin $< 1.0$ second). While internet-connected nodes sync via standard NTP servers, the system includes air-gapped fallback options: automated synchronization over-the-air (OTA) via dedicated radio beacons or manual calibration via time-signal radio broadcasts.
* **Hierarchical Topology:** The network topology is architected as a managed cluster consisting of **1 Master (Command/Hub) Station and up to 45 Subordinate (Field) Nodes** split into 3 distinct functional groups.

---

## 🛡️ Functional Capabilities

* **Large Network Scaling:** Supports up to 45 active radio stations in a single area, with up to 15 nodes capable of simultaneous transmission without interfering with each other.
* **Flexible Messaging System:** Handles automated transmission, receipt, and local database storage of short messages (up to 24 characters) and long payloads (up to 384 characters), complete with automated timestamping, sender IDs, and recipient IDs.
* **Hardware-Enforced Cryptography ("Enigma" Layer):** To prevent eavesdropping, all transmitted payloads are pre-encrypted before modulation. The encryption utilizes a digital emulation of the polyalphabetic **Enigma rotor cipher** driven by a unique 3-digit key. The cryptographic keys reside exclusively inside a secure, tamper-resistant **hardware USB dongle**. The node software is completely locked and inoperable unless this physical electronic key is present.
* **Real-Time Link Quality Analysis (LQA):** The software automatically monitors ionospheric radio propagation and SNR levels between the Master station and all 45 Subordinate nodes during a tight 1-minute cycle, rendering an intuitive, glanceable connectivity map.
* **Fully Concurrent Operations:** Network health monitoring, automatic synchronization of drifting nodes, and active text traffic routing occur **simultaneously and seamlessly** within the protocol stack.

---

## ⚙️ Technical Implementation & Network Protocol

### 1. Hardware Interface
Physical connection to the radio transceiver is minimal and relies on standard analog audio interfaces. The computer connects to the radio via just two links: the **Microphone/Line-In** jack for reception and the **Headphone/Line-Out** jack for transmission. Push-To-Talk (PTT) is handled via automated VOX or audio-triggered interface lines.

### 2. Hybrid TDMA / FDMA Time-Slot Allocation
The system divides every 1 minute of radio time into four discrete **15-second time slots**. Nodes operate in strict simplex mode: they cannot receive while transmitting.

| Time Slot | Duration | Allocated Network Activity | Operational Mode |
| :--- | :--- | :--- | :--- |
| **Slot 1** | 00s - 15s | **Master (Command) Station Transmission** | All 45 Nodes listen; Master sends commands/sync. |
| **Slot 2** | 15s - 30s | **Subordinate Group A (Nodes 1–15)** | FDMA Parallel Transmission / Master listens. |
| **Slot 3** | 30s - 45s | **Subordinate Group B (Nodes 16–30)** | FDMA Parallel Transmission / Master listens. |
| **Slot 4** | 45s - 60s | **Subordinate Group C (Nodes 31–45)** | FDMA Parallel Transmission / Master listens. |

### 3. FDMA Subcarrier Mapping & Routing
Within each 15-second slot assigned to a Group, the 3 kHz audio spectrum is divided into 15 individual frequency subchannels. Each of the 15 radio stations in a group is permanently assigned to its own specific frequency slot. 
* **Sender Identification:** The receiving station immediately identifies the transmitting node based on the precise frequency channel the signal arrives on.
* **Recipient Routing:** The target recipient's address code is embedded directly into the encrypted payload header, hidden from third-party interception.

### 4. Dynamic Bandwidth Allocation (Short vs. Long Messages)
The system distinguishes payloads on the fly to maximize channel efficiency and prevent collisions:
* **Short Messages ($\le$ 24 characters):** These use narrow-band subcarrier clusters. Because of the strict FDMA channel separation, all 15 radio stations within a group can transmit simultaneously in the same time slot without any mutual interference.
* **Long Messages ($>$ 24 characters):** These expand to occupy the entire available 3 kHz frequency bandwidth to achieve maximum data throughput. To prevent data corruption, the system runs an automated scheduling algorithm: when a long message is queued, all other nodes in that specific group are automatically instructed to remain silent during that time slot. This throttling mechanism is fully automated and completely invisible to the operators.
* **Automated Telemetry:** Channel SNR monitoring and link quality checks run automatically on a background schedule, using dedicated micro-windows calculated by the protocol without disrupting active text traffic.

---

## 🛠️ Development & Distribution Status

1. **Protocol Testing Stage:** This software is explicitly designed for protocol testing and verification. As a result, it includes comprehensive debug log outputs and evaluation interface tools.
2. **Binary Distribution:** At this initial stage, the software is provided as compiled executable files only. If there is sufficient interest and backing from the community, the full C++ source code will be made open-source.
3. **Interface Previews:** Demonstrations and screenshots of the operational interface screens are available in the `SCREENS` folder.

---

## 📄 Credits & Acknowledgments

This software architecture is a deeply customized, multichannel parallel extension wrapper built around the open-source low-level FT8 C library core developed by **Karlis Goba (YL3JG)** (available at [kgoba/ft8_lib](https://github.com)).

* **Original Core:** Handles raw 77-bit packing, Forward Error Correction (FEC) via LDPC matrices, and basic tone modulation/demodulation.
This implementation: Implements parallel multi-frequency channel multiplexing, automated TDMA/FDMA cluster scheduling for 45 nodes, interfacing with the "Enigma" cryptographic device, and the automated radio-sync stack.

---

## ⚖️ License

This project is licensed under the **GNU General Public License v3 (GPL-3.0)**. Commercial closed-source cloning, proprietary distribution, or stripping of the open-source copyleft protection is strictly prohibited under international copyright laws.
