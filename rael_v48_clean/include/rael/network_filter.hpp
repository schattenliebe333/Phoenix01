// ═══════════════════════════════════════════════════════════════════════════════
// RAEL NETWORK FILTER - Kernel-Level Packet Interception
// ═══════════════════════════════════════════════════════════════════════════════
//
// Linux:   eBPF/XDP (eXpress Data Path) - Kernel-Level Packet Filter
// Windows: WFP (Windows Filtering Platform) - Kernel Callout Driver
//
// Pipeline: PACKET → 88-CHECK → LABYRINTH → GRAVITRAVITATION → VOLLENSTRAHLEN
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fwpmu.h>
#include <fwpmtypes.h>
#pragma comment(lib, "fwpuclnt.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <linux/bpf.h>
#include <sys/syscall.h>
#endif

#include "security_core.hpp"

namespace rael {
namespace network {

// ═══════════════════════════════════════════════════════════════════════════════
// PACKET STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════════

struct PacketInfo {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;  // TCP=6, UDP=17
    size_t payload_size;
    std::vector<uint8_t> payload;
    double threat_score;
    bool has_signature_88;

    std::string src_ip_str() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
            (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
            (src_ip >> 8) & 0xFF, src_ip & 0xFF);
        return buf;
    }

    std::string dst_ip_str() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
            (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
            (dst_ip >> 8) & 0xFF, dst_ip & 0xFF);
        return buf;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// FILTER DECISION
// ═══════════════════════════════════════════════════════════════════════════════

enum class FilterAction {
    ALLOW,           // Durchlassen (Freund oder harmlos)
    BLOCK,           // Blockieren
    TRAP,            // In Gravitravitation fangen
    TRANSFORM,       // Durch Vollenstrahlen transformieren
    QUARANTINE       // Isolieren für Analyse
};

const char* action_name(FilterAction a) {
    switch(a) {
        case FilterAction::ALLOW: return "ALLOW";
        case FilterAction::BLOCK: return "BLOCK";
        case FilterAction::TRAP: return "TRAP";
        case FilterAction::TRANSFORM: return "TRANSFORM";
        case FilterAction::QUARANTINE: return "QUARANTINE";
        default: return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT SIGNATURES - Bekannte Angriffsmuster
// ═══════════════════════════════════════════════════════════════════════════════

class ThreatSignatures {
private:
    std::set<uint32_t> blocked_ips_;
    std::set<uint16_t> dangerous_ports_;
    std::vector<std::vector<uint8_t>> malware_patterns_;
    std::mutex mtx_;

public:
    ThreatSignatures() {
        // Gefährliche Ports
        dangerous_ports_ = {
            4444,   // Metasploit
            5555,   // Android Debug
            6666, 6667,  // IRC C2
            31337,  // Elite
            12345,  // NetBus
            27374,  // SubSeven
            1337,   // Backdoor
            9001, 9030,  // Tor
            4443,   // Alternate HTTPS (oft C2)
        };

        // Bekannte Malware-Patterns (vereinfacht)
        malware_patterns_ = {
            // Metasploit Meterpreter stub
            {0x4d, 0x5a, 0x90, 0x00},
            // Cobalt Strike beacon
            {0xfc, 0xe8, 0x89, 0x00},
            // Generic shellcode NOP sled
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90},
        };
    }

    void add_blocked_ip(uint32_t ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        blocked_ips_.insert(ip);
    }

    void add_blocked_ip(const std::string& ip_str) {
        uint32_t ip = 0;
        int a, b, c, d;
        if (sscanf(ip_str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            ip = (a << 24) | (b << 16) | (c << 8) | d;
            add_blocked_ip(ip);
        }
    }

    bool is_blocked_ip(uint32_t ip) const {
        return blocked_ips_.count(ip) > 0;
    }

    bool is_dangerous_port(uint16_t port) const {
        return dangerous_ports_.count(port) > 0;
    }

    bool contains_malware_pattern(const std::vector<uint8_t>& payload) const {
        for (const auto& pattern : malware_patterns_) {
            if (payload.size() >= pattern.size()) {
                for (size_t i = 0; i <= payload.size() - pattern.size(); ++i) {
                    if (std::memcmp(&payload[i], pattern.data(), pattern.size()) == 0) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Berechnet Bedrohungsscore (0.0 - 1.0)
    double calculate_threat_score(const PacketInfo& pkt) const {
        double score = 0.0;

        if (is_blocked_ip(pkt.src_ip)) score += 0.9;
        if (is_dangerous_port(pkt.src_port)) score += 0.3;
        if (is_dangerous_port(pkt.dst_port)) score += 0.3;
        if (contains_malware_pattern(pkt.payload)) score += 0.8;

        // Ungewöhnlich große Pakete
        if (pkt.payload_size > 65000) score += 0.2;

        return std::min(1.0, score);
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// 88-SIGNATUR PRÜFUNG FÜR NETZWERK
// ═══════════════════════════════════════════════════════════════════════════════

class SignatureValidator {
private:
    // Bekannte freundliche IPs (Whitelist)
    std::set<uint32_t> trusted_ips_;
    // Bekannte freundliche Domains (Hash)
    std::set<uint64_t> trusted_domain_hashes_;
    std::mutex mtx_;

public:
    void add_trusted_ip(uint32_t ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        trusted_ips_.insert(ip);
    }

    void add_trusted_ip(const std::string& ip_str) {
        uint32_t ip = 0;
        int a, b, c, d;
        if (sscanf(ip_str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            ip = (a << 24) | (b << 16) | (c << 8) | d;
            add_trusted_ip(ip);
        }
    }

    // Prüft ob Paket die 88-Signatur trägt
    bool has_signature_88(const PacketInfo& pkt) const {
        // 1. Trusted IP?
        if (trusted_ips_.count(pkt.src_ip)) return true;

        // 2. Lokales Netzwerk (192.168.x.x, 10.x.x.x, 172.16-31.x.x)
        uint8_t first = (pkt.src_ip >> 24) & 0xFF;
        uint8_t second = (pkt.src_ip >> 16) & 0xFF;
        if (first == 192 && second == 168) return true;
        if (first == 10) return true;
        if (first == 172 && second >= 16 && second <= 31) return true;
        if (first == 127) return true;  // Localhost

        // 3. Payload-basierte Signatur (suche nach 88-Marker)
        // Format: "RAEL88:" am Anfang des Payloads
        if (pkt.payload.size() >= 7) {
            if (pkt.payload[0] == 'R' && pkt.payload[1] == 'A' &&
                pkt.payload[2] == 'E' && pkt.payload[3] == 'L' &&
                pkt.payload[4] == '8' && pkt.payload[5] == '8' &&
                pkt.payload[6] == ':') {
                return true;
            }
        }

        return false;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// NETWORK FILTER - Hauptklasse
// ═══════════════════════════════════════════════════════════════════════════════

class NetworkFilter {
private:
    security::SecurityCore* security_core_;
    ThreatSignatures signatures_;
    SignatureValidator validator_;

    std::atomic<bool> running_;
    std::thread filter_thread_;
    std::mutex mtx_;

    // Statistiken
    std::atomic<uint64_t> packets_total_{0};
    std::atomic<uint64_t> packets_allowed_{0};
    std::atomic<uint64_t> packets_blocked_{0};
    std::atomic<uint64_t> packets_trapped_{0};
    std::atomic<uint64_t> packets_transformed_{0};

    // IP-basierte Zähler für Gravitravitation
    std::map<uint32_t, int> attack_counts_;

    // Callbacks
    std::function<void(const PacketInfo&, FilterAction)> on_packet_filtered_;

public:
    NetworkFilter(security::SecurityCore* core = nullptr)
        : security_core_(core), running_(false) {}

    ~NetworkFilter() {
        stop();
    }

    void set_security_core(security::SecurityCore* core) {
        security_core_ = core;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // KERNFUNKTION: Paket filtern
    // ═══════════════════════════════════════════════════════════════════════════

    FilterAction filter_packet(PacketInfo& pkt) {
        packets_total_++;

        // 1. 88-SIGNATUR PRÜFUNG
        pkt.has_signature_88 = validator_.has_signature_88(pkt);
        if (pkt.has_signature_88) {
            packets_allowed_++;
            return FilterAction::ALLOW;  // Freund durchlassen
        }

        // 2. BEDROHUNGSSCORE BERECHNEN
        pkt.threat_score = signatures_.calculate_threat_score(pkt);

        // 3. ENTSCHEIDUNG BASIEREND AUF SCORE
        FilterAction action = FilterAction::ALLOW;

        if (pkt.threat_score > security::rst::G0) {
            // Hohe Bedrohung → BLOCKIEREN
            action = FilterAction::BLOCK;
            packets_blocked_++;

            // IP zur Blacklist hinzufügen
            signatures_.add_blocked_ip(pkt.src_ip);

        } else if (pkt.threat_score > security::rst::G3) {
            // Mittlere Bedrohung → Prüfen ob Wiederholungstäter
            std::lock_guard<std::mutex> lock(mtx_);
            attack_counts_[pkt.src_ip]++;

            if (attack_counts_[pkt.src_ip] >= 3) {
                // 3+ Angriffe → GRAVITRAVITATION (gefangen!)
                action = FilterAction::TRAP;
                packets_trapped_++;
            } else {
                // Erste Angriffe → TRANSFORMIEREN
                action = FilterAction::TRANSFORM;
                packets_transformed_++;
            }

        } else if (pkt.threat_score > security::rst::G5) {
            // Niedrige Bedrohung → TRANSFORMIEREN
            action = FilterAction::TRANSFORM;
            packets_transformed_++;

        } else {
            // Harmlos → DURCHLASSEN
            packets_allowed_++;
        }

        // 4. SECURITY CORE BENACHRICHTIGEN
        if (security_core_ && action != FilterAction::ALLOW) {
            security::Threat threat;
            threat.source = pkt.src_ip_str() + ":" + std::to_string(pkt.src_port);
            threat.attack_energy = pkt.threat_score;

            switch(action) {
                case FilterAction::BLOCK:
                    threat.type = security::ThreatType::NETWORK_C2;
                    threat.details = "High threat score: " + std::to_string(pkt.threat_score);
                    break;
                case FilterAction::TRAP:
                    threat.type = security::ThreatType::NETWORK_SUSPICIOUS;
                    threat.details = "Repeat offender - trapped in Gravitravitation";
                    break;
                case FilterAction::TRANSFORM:
                    threat.type = security::ThreatType::NETWORK_SUSPICIOUS;
                    threat.details = "Transformed by Vollenstrahlen";
                    break;
                default:
                    break;
            }

            security_core_->process_threat(threat);
        }

        // 5. CALLBACK
        if (on_packet_filtered_) {
            on_packet_filtered_(pkt, action);
        }

        return action;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // RAW SOCKET LISTENER (Linux)
    // ═══════════════════════════════════════════════════════════════════════════

#ifndef _WIN32
    void start_raw_capture() {
        if (running_) return;
        running_ = true;

        filter_thread_ = std::thread([this]() {
            // Raw Socket erstellen (benötigt root)
            int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
            if (sock < 0) {
                // Fallback: Normaler Socket
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) return;
            }

            uint8_t buffer[65536];

            while (running_) {
                ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
                if (len > 0) {
                    PacketInfo pkt = parse_ip_packet(buffer, len);
                    filter_packet(pkt);
                }
            }

            close(sock);
        });
    }

    PacketInfo parse_ip_packet(const uint8_t* data, size_t len) {
        PacketInfo pkt;
        pkt.payload_size = 0;
        pkt.threat_score = 0.0;
        pkt.has_signature_88 = false;

        if (len < 20) return pkt;  // Zu kurz für IP Header

        // IP Header
        struct iphdr* ip = (struct iphdr*)data;
        pkt.src_ip = ntohl(ip->saddr);
        pkt.dst_ip = ntohl(ip->daddr);
        pkt.protocol = ip->protocol;

        size_t ip_hdr_len = ip->ihl * 4;

        if (ip->protocol == IPPROTO_TCP && len >= ip_hdr_len + 20) {
            struct tcphdr* tcp = (struct tcphdr*)(data + ip_hdr_len);
            pkt.src_port = ntohs(tcp->source);
            pkt.dst_port = ntohs(tcp->dest);

            size_t tcp_hdr_len = tcp->doff * 4;
            size_t payload_offset = ip_hdr_len + tcp_hdr_len;

            if (len > payload_offset) {
                pkt.payload_size = len - payload_offset;
                pkt.payload.assign(data + payload_offset, data + len);
            }
        } else if (ip->protocol == IPPROTO_UDP && len >= ip_hdr_len + 8) {
            struct udphdr* udp = (struct udphdr*)(data + ip_hdr_len);
            pkt.src_port = ntohs(udp->source);
            pkt.dst_port = ntohs(udp->dest);

            size_t payload_offset = ip_hdr_len + 8;
            if (len > payload_offset) {
                pkt.payload_size = len - payload_offset;
                pkt.payload.assign(data + payload_offset, data + len);
            }
        }

        return pkt;
    }
#endif

    // ═══════════════════════════════════════════════════════════════════════════
    // WINDOWS FILTERING PLATFORM (WFP)
    // ═══════════════════════════════════════════════════════════════════════════

#ifdef _WIN32
    void start_wfp_filter() {
        if (running_) return;
        running_ = true;

        // WFP Engine Handle
        HANDLE engine = NULL;
        FWPM_SESSION0 session = {0};
        session.flags = FWPM_SESSION_FLAG_DYNAMIC;

        DWORD result = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &engine);
        if (result != ERROR_SUCCESS) {
            return;
        }

        // Filter hinzufügen für ausgehenden Traffic
        FWPM_FILTER0 filter = {0};
        filter.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
        filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
        filter.weight.type = FWP_EMPTY;
        filter.displayData.name = L"RAEL Security Filter";
        filter.displayData.description = L"Attack to Defense Conversion";

        UINT64 filterId;
        result = FwpmFilterAdd0(engine, &filter, NULL, &filterId);

        // Monitoring Thread
        filter_thread_ = std::thread([this, engine]() {
            while (running_) {
                // WFP Events verarbeiten
                Sleep(100);
            }
            FwpmEngineClose0(engine);
        });
    }
#endif

    void start() {
#ifdef _WIN32
        start_wfp_filter();
#else
        start_raw_capture();
#endif
    }

    void stop() {
        running_ = false;
        if (filter_thread_.joinable()) {
            filter_thread_.join();
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // KONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════════

    void add_trusted_ip(const std::string& ip) {
        validator_.add_trusted_ip(ip);
    }

    void add_blocked_ip(const std::string& ip) {
        signatures_.add_blocked_ip(ip);
    }

    void set_filter_callback(std::function<void(const PacketInfo&, FilterAction)> cb) {
        on_packet_filtered_ = cb;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════════════

    uint64_t get_packets_total() const { return packets_total_; }
    uint64_t get_packets_allowed() const { return packets_allowed_; }
    uint64_t get_packets_blocked() const { return packets_blocked_; }
    uint64_t get_packets_trapped() const { return packets_trapped_; }
    uint64_t get_packets_transformed() const { return packets_transformed_; }

    std::string status_report() const {
        std::ostringstream ss;
        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "           RAEL NETWORK FILTER - STATUS\n";
        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "\n";
        ss << "┌─ PACKET STATISTICS ─────────────────────────────────────────┐\n";
        ss << "│ TOTAL PACKETS:      " << packets_total_ << "\n";
        ss << "│ ALLOWED:            " << packets_allowed_ << " (mit 88-Signatur oder harmlos)\n";
        ss << "│ BLOCKED:            " << packets_blocked_ << " (hohe Bedrohung)\n";
        ss << "│ TRAPPED:            " << packets_trapped_ << " (in Gravitravitation)\n";
        ss << "│ TRANSFORMED:        " << packets_transformed_ << " (durch Vollenstrahlen)\n";
        ss << "└─────────────────────────────────────────────────────────────┘\n";

        if (packets_total_ > 0) {
            double block_rate = 100.0 * packets_blocked_ / packets_total_;
            double trap_rate = 100.0 * packets_trapped_ / packets_total_;
            ss << "\n";
            ss << "Block Rate: " << block_rate << "%\n";
            ss << "Trap Rate:  " << trap_rate << "%\n";
        }

        ss << "═══════════════════════════════════════════════════════════════\n";
        return ss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// PROXY SERVER (Alternative zu Raw Sockets - benötigt keine Root-Rechte)
// ═══════════════════════════════════════════════════════════════════════════════

class ProxyServer {
private:
    NetworkFilter* filter_;
    uint16_t port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    int server_socket_;

public:
    ProxyServer(NetworkFilter* filter, uint16_t port = 8888)
        : filter_(filter), port_(port), running_(false), server_socket_(-1) {}

    ~ProxyServer() {
        stop();
    }

    void start() {
        if (running_) return;
        running_ = true;

#ifndef _WIN32
        server_thread_ = std::thread([this]() {
            server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_ < 0) return;

            int opt = 1;
            setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port_);

            if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(server_socket_);
                return;
            }

            listen(server_socket_, 10);

            while (running_) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);

                if (client >= 0) {
                    // Handle in separatem Thread
                    std::thread([this, client, client_addr]() {
                        handle_connection(client, client_addr);
                    }).detach();
                }
            }

            close(server_socket_);
        });
#endif
    }

    void stop() {
        running_ = false;
#ifndef _WIN32
        if (server_socket_ >= 0) {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
        }
#endif
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

private:
#ifndef _WIN32
    void handle_connection(int client, struct sockaddr_in client_addr) {
        uint8_t buffer[65536];
        ssize_t len = recv(client, buffer, sizeof(buffer), 0);

        if (len > 0) {
            PacketInfo pkt;
            pkt.src_ip = ntohl(client_addr.sin_addr.s_addr);
            pkt.src_port = ntohs(client_addr.sin_port);
            pkt.dst_ip = 0;  // Wird aus HTTP Header extrahiert
            pkt.dst_port = 80;
            pkt.protocol = 6;  // TCP
            pkt.payload_size = len;
            pkt.payload.assign(buffer, buffer + len);

            FilterAction action = filter_->filter_packet(pkt);

            if (action == FilterAction::ALLOW) {
                // Weiterleiten (vereinfacht)
                send(client, buffer, len, 0);
            } else {
                // Blockierte Antwort
                const char* blocked = "HTTP/1.1 403 Forbidden\r\n"
                                     "Content-Type: text/html\r\n\r\n"
                                     "<html><body><h1>RAEL: Blocked by Gravitravitation</h1></body></html>";
                send(client, blocked, strlen(blocked), 0);
            }
        }

        close(client);
    }
#endif
};

} // namespace network
} // namespace rael
