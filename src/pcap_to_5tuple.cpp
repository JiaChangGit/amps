// g++ -std=c++17 -o pcap_to_5tuple pcap_to_5tuple.cpp -lpcap

#include <pcap.h>
#include <iostream>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>

// 將 IP 結構轉成 uint32_t 整數（host byte order）
uint32_t ip_to_uint32(struct in_addr ip_addr) {
    return ntohl(ip_addr.s_addr); // big endian → host endian
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./pcap_to_5tuple input.pcap output.txt\n";
        return 1;
    }

    const char* pcap_file = argv[1];
    const char* output_file = argv[2];
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_offline(pcap_file, errbuf);

    if (!handle) {
        std::cerr << "Error opening file: " << errbuf << "\n";
        return 1;
    }

    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    struct pcap_pkthdr* header;
    const u_char* data;
    int link_type = pcap_datalink(handle);

    while (pcap_next_ex(handle, &header, &data) >= 0) {
        const struct ip* ip_hdr = nullptr;
        const struct tcphdr* tcp_hdr = nullptr;
        const struct udphdr* udp_hdr = nullptr;

        // 根據 link-layer 偏移（此為 Ethernet，MAC header 14 bytes）
        if (link_type == DLT_EN10MB) {
            ip_hdr = (struct ip*)(data + 14);
        } else {
            std::cerr << "Unsupported link type: " << link_type << "\n";
            continue;
        }

        // 只處理 IPv4 封包
        if (ip_hdr->ip_v != 4) continue;

        uint32_t src_ip = ip_to_uint32(ip_hdr->ip_src);
        uint32_t dst_ip = ip_to_uint32(ip_hdr->ip_dst);
        uint8_t proto = ip_hdr->ip_p;
        uint16_t src_port = 0, dst_port = 0;

        size_t ip_header_len = ip_hdr->ip_hl * 4;
        const u_char* transport_hdr = (u_char*)ip_hdr + ip_header_len;

        if (proto == IPPROTO_TCP) {
            tcp_hdr = (struct tcphdr*)transport_hdr;
            src_port = ntohs(tcp_hdr->source);
            dst_port = ntohs(tcp_hdr->dest);
        } else if (proto == IPPROTO_UDP) {
            udp_hdr = (struct udphdr*)transport_hdr;
            src_port = ntohs(udp_hdr->source);
            dst_port = ntohs(udp_hdr->dest);
        } else {
            continue; // 忽略非 TCP/UDP
        }

        // 寫入數字格式的七欄資料
        out << src_ip << "\t" << dst_ip << "\t"
            << src_port << "\t" << dst_port << "\t"
            << (int)proto << "\t0\t0\n";
    }

    pcap_close(handle);
    out.close();
    std::cout << "✅ Export done: " << output_file << "\n";
    return 0;
}

