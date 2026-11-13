# FT8-Based Automated Advanced Tactical HF Modem (ft8-text-link-auto)

An advanced, high-efficiency, and highly automated tactical High Frequency (HF) data communication network protocol. This system leverages the FT8 protocol's extreme weak-signal processing engine, scaled up into an advanced multi-channel data link layout.

This software transforms a standard HF/VHF transceiver into an automated text messenger capable of reliable data exchange under heavy noise and ionospheric fading, requiring no operator intervention for link establishment, clock correction, or network routing.


## 📑 Architectural Manifesto & Technical Priority (FT8-TAP)

In 2025, the concept of multi-channel FT8 multiplexing for tactical MANET (Mobile Ad-hoc Networks) was conceptualized and developed by the author. This architecture allows the transmission of up to 384 characters instead of the standard 13 characters within a compressed time frame by concurrently processing multiple subcarriers. 

Alongside this breakthrough, a specialized tactical radio cluster protocol was engineered, establishing fully automated, autonomous communication infrastructure for up to 46 radio stations. This protocol is officially designated as FT8-TAP (Tactical Advanced Protocol).


## 📑 Citation & Academic/Technical Reference

If you are using this protocol for testing, field operations, radio research, or referencing its architecture in your own projects, you must credit the author. 

Please use the following format for citations and references:
Andrew Zaharov (andrew-v-z), "Advanced Multi-Channel Tactical HF Modem (ft8-text-link-auto)", GitHub Repository, 2026.


## ⚡ Core Concepts & Theoretical Foundation

• Deep Noise Penetration: The core modem utilizes the underlying mathematical engine of FT8, allowing error-free data decoding at Signal-to-Noise Ratios (SNR) down to -20 dB — conditions where human voice or standard digital modes fail completely.

• Audio-Bandwidth Multiplexing: All data processing, modulation, and demodulation take place within a standard 3 kHz Single Sideband (SSB) voice bandwidth, making it compatible with any conventional analog radio transceiver.

• Burst Packetized Transmission: Data is encapsulated into tight, highly optimized burst packets.

• Advanced Capacity (Multi-Channel Scaling): Standard FT8 transmits 12–13 characters (77 bits) per 15-second block. This system implements frequency-domain multiplexing to scale up payloads, allowing a single message to carry up to 384 characters within a compressed time-slot window.

• The Shannon Limit Trade-off: In strict accordance with Shannon's Channel Capacity Theorem, increasing the text payload volume within a tight timeframe splits the transceiver's total output power. Hence, a 24-character message yields maximum "punching power" at -20 dB, while a maximum 384-character payload requires a relatively cleaner channel.

• Strict Time-Slot Synchronization: The protocol relies on precise time synchronization between nodes (error margin < 1.0 second). While internet-connected nodes sync via standard NTP servers, the system includes air-gapped fallback options: automated synchronization over-the-air (OTA) via dedicated radio beacons or manual calibration via time-signal radio broadcasts.

• Hierarchical Topology: The network topology is architected as a managed cluster consisting of 1 Master (Command/Hub) Station and up to 45 Subordinate (Field) Nodes.


## 🛡️ Functional Capabilities

• Large Network Scaling: Supports up to 45 active radio stations in a single area with automated concurrent node management to ensure zero mutual interference.

• Flexible Messaging System: Handles automated transmission, receipt, and local database storage of short messages (up to 24 characters) and long payloads (up to 384 characters), complete with automated timestamping, sender IDs, and recipient IDs.

• Integrated Cryptography & Node Security: To prevent unauthorized eavesdropping, all transmitted payloads are pre-encrypted before modulation using a secure cryptographic subsystem. In production releases, these network identity parameters, encryption keys, and the specific node role (Master/Subordinate) are designed to be hard-coded and managed via an external physical hardware security token (USB dongle).

• Real-Time Link Quality Analysis (LQA): The software automatically monitors ionospheric radio propagation and SNR levels between the Master station and all 45 Subordinate nodes, rendering an intuitive, glanceable connectivity map in real time.

• Fully Concurrent Operations: Network health monitoring, automatic synchronization of drifting nodes, and active text traffic routing occur simultaneously and seamlessly within the protocol stack.


## ⚙️ Technical Implementation

Network Coordination & Hardware Interface

1. Master-Controlled Traffic: All transmission windows and data scheduling are dynamically regulated by the Master station to completely eliminate air collisions.

2. Hardware Connection: Physical connection to the radio transceiver is minimal and relies on standard analog audio interfaces. The computer connects to the radio via just two links: the Microphone/Line-In jack for reception and the Headphone/Line-Out jack for transmission. Push-To-Talk (PTT) is handled via automated VOX or audio-triggered interface lines.

3. No soldering needed for modern rigs: An external hardware VOX is optional if your transceiver has built-in DATA VOX and adjustable RX/TX audio gain (tested with Yaesu FT-891).


## 🛠️ Development & Distribution Status

1. Protocol Testing Stage: This software is explicitly designed for protocol testing and verification. As a result, it includes comprehensive debug log outputs and evaluation interface tools.

2. Binary Distribution: At this initial stage, the software is provided as compiled executable files only. If there is sufficient interest and backing from the community, the full C++ source code will be made open-source.

3. Interface Previews: Demonstrations and screenshots of the operational interface screens are available in the SCREENS folder.


## 📄 Credits & Acknowledgments

This software architecture is a deeply customized, multi-channel network infrastructure built around the open-source low-level FT8 C library core developed by Karlis Goba (YL3JG) (available at kgoba/ft8_lib).

• Original Core: Handles raw 77-bit packing, Forward Error Correction (FEC) via LDPC matrices, and basic tone modulation/demodulation.

• This Implementation: Introduces a proprietary multi-channel routing stack, secure hardware token validation, automated network cluster scheduling, and an air-gapped radio-synchronization engine.


## ⚖️ License

This project is licensed under the GNU General Public License v3 (GPL-3.0). Commercial closed-source cloning, proprietary distribution, or stripping of the open-source copyleft protection is strictly prohibited under international copyright laws.
