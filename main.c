#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <wininet.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wininet.lib")

void print_header() {
    printf("netool - Network Utility Tool\n");
    printf("Developers: ExEintel\n");
    printf("================================\n\n");
}

void show_usage() {
    printf("Usage: netool <command> [options]\n\n");
    printf("Basic Diagnostics:\n");
    printf("  netool myip              Show public IP\n");
    printf("  netool myip --details    Show public IP with details\n");
    printf("  netool localip           Show local IP addresses\n");
    printf("  netool iface             List network interfaces\n");
    printf("  netool dns               Show DNS servers\n\n");
    printf("Connection Management:\n");
    printf("  netool renew             Renew DHCP lease\n");
    printf("  netool reconnect         Reconnect active interface\n\n");
    printf("Ping & Connectivity:\n");
    printf("  netool ping <host>       Ping host (4 packets)\n");
    printf("  netool ping -t <host>    Continuous ping\n");
    printf("  netool trace <host>      Traceroute\n\n");
    printf("Ports & Services:\n");
    printf("  netool ports --open      Show open ports\n");
    printf("  netool scan <host>       Quick port scan\n\n");
    printf("Connection Control:\n");
    printf("  netool online            Check internet connectivity\n");
    printf("  netool speed             Speed test\n\n");
    printf("Advanced:\n");
    printf("  netool whois <domain>    WHOIS lookup\n");
    printf("  netool route             Show routing table\n");
    printf("  netool flushdns          Flush DNS cache\n");
}

char* get_adapter_type_string(DWORD type) {
    switch (type) {
        case IF_TYPE_ETHERNET_CSMACD: return "Ethernet";
        case IF_TYPE_IEEE80211: return "WiFi";
        case IF_TYPE_SOFTWARE_LOOPBACK: return "Loopback";
        case IF_TYPE_PPP: return "PPP";
        default: return "Other";
    }
}

char* get_oper_status_string(IF_OPER_STATUS status) {
    switch (status) {
        case IfOperStatusUp: return "up";
        case IfOperStatusDown: return "down";
        case IfOperStatusTesting: return "testing";
        default: return "unknown";
    }
}

void cmd_myip(int details) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    HINTERNET hInternet = InternetOpenA("netool", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Error: Failed to initialize\n");
        WSACleanup();
        return;
    }
    
    if (details) {
        HINTERNET hUrl = InternetOpenUrlA(hInternet, "http://ip-api.com/line/?fields=query,isp,city,country", NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hUrl) {
            char buffer[4096];
            DWORD bytesRead;
            if (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead)) {
                buffer[bytesRead] = '\0';
                char* line = strtok(buffer, "\n");
                printf("IPv4: %s", line ? line : "N/A");
                line = strtok(NULL, "\n");
                printf(" (Provider: %s", line ? line : "N/A");
                line = strtok(NULL, "\n");
                printf(", City: %s", line ? line : "N/A");
                line = strtok(NULL, "\n");
                printf(", Country: %s)\n", line ? line : "N/A");
            }
            InternetCloseHandle(hUrl);
        } else {
            printf("Error: Could not fetch details\n");
        }
    } else {
        const char* services[] = {"http://api.ipify.org", "http://ipinfo.io/ip", "http://checkip.amazonaws.com"};
        int success = 0;
        
        for (int i = 0; i < 3 && !success; i++) {
            HINTERNET hUrl = InternetOpenUrlA(hInternet, services[i], NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hUrl) {
                char buffer[256];
                DWORD bytesRead;
                if (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead)) {
                    buffer[bytesRead] = '\0';
                    printf("IPv4: %s\n", buffer);
                    success = 1;
                }
                InternetCloseHandle(hUrl);
            }
        }
        
        if (!success) {
            printf("Error: Could not fetch public IP\n");
        }
    }
    
    InternetCloseHandle(hInternet);
    WSACleanup();
}

void cmd_localip() {
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
        if (!pAddresses) {
            printf("Error: Memory allocation failed\n");
            return;
        }
        
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen);
        
        if (dwRetVal == NO_ERROR) {
            PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
            while (pCurr) {
                if (pCurr->OperStatus == IfOperStatusUp && pCurr->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
                    printf("%S: ", pCurr->FriendlyName);
                    
                    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                    int first = 1;
                    while (pUnicast) {
                        char ipstr[INET6_ADDRSTRLEN];
                        struct sockaddr* sa = pUnicast->Address.lpSockaddr;
                        
                        if (sa->sa_family == AF_INET) {
                            struct sockaddr_in* sin = (struct sockaddr_in*)sa;
                            inet_ntop(AF_INET, &sin->sin_addr, ipstr, sizeof(ipstr));
                            if (!first) printf(", ");
                            printf("%s", ipstr);
                            first = 0;
                        }
                        pUnicast = pUnicast->Next;
                    }
                    printf("\n");
                }
                pCurr = pCurr->Next;
            }
        }
        free(pAddresses);
    }
}

void cmd_iface() {
    ULONG outBufLen = 0;
    
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
        if (!pAddresses) return;
        
        if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen) == NO_ERROR) {
            PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
            int i = 1;
            while (pCurr) {
                printf("%d. %S (%s) - %s\n", i++, pCurr->FriendlyName, 
                       get_adapter_type_string(pCurr->IfType), 
                       get_oper_status_string(pCurr->OperStatus));
                pCurr = pCurr->Next;
            }
        }
        free(pAddresses);
    }
}

void cmd_dns() {
    FIXED_INFO* pFixedInfo = NULL;
    ULONG ulOutBufLen = 0;
    
    if (GetNetworkParams(NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        pFixedInfo = (FIXED_INFO*)malloc(ulOutBufLen);
        if (pFixedInfo) {
            if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR) {
                IP_ADDR_STRING* pIPAddr = &pFixedInfo->DnsServerList;
                int i = 1;
                while (pIPAddr) {
                    printf("DNS%d: %s\n", i++, pIPAddr->IpAddress.String);
                    pIPAddr = pIPAddr->Next;
                }
            }
            free(pFixedInfo);
        }
    }
}

void cmd_renew() {
    printf("Renewing DHCP lease...\n");
    system("ipconfig /renew > nul 2>&1");
    printf("Done. Use 'ipconfig /all' to see new addresses.\n");
}

void cmd_reconnect() {
    printf("Reconnecting active interface...\n");
    printf("Note: This requires administrator privileges.\n");
    system("ipconfig /release > nul 2>&1 && ipconfig /renew > nul 2>&1");
    printf("Done.\n");
}

void cmd_ping(char* host, int continuous) {
    if (continuous) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ping -t %s", host);
        system(cmd);
    } else {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ping -n 4 %s", host);
        system(cmd);
    }
}

void cmd_trace(char* host) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "tracert %s", host);
    system(cmd);
}

void cmd_ports_open() {
    printf("Open ports on this system:\n");
    system("netstat -an | findstr LISTENING");
}

int is_port_open(char* host, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 0;
    }
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, host, &server.sin_addr);
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    int result = connect(sock, (struct sockaddr*)&server, sizeof(server));
    closesocket(sock);
    WSACleanup();
    
    return (result == 0);
}

void cmd_scan(char* host) {
    printf("Scanning %s (common ports)...\n", host);
    
    int ports[] = {21, 22, 23, 25, 53, 80, 110, 143, 443, 445, 3389};
    int num = sizeof(ports) / sizeof(ports[0]);
    
    printf("Open ports:\n");
    int found = 0;
    
    for (int i = 0; i < num; i++) {
        if (is_port_open(host, ports[i])) {
            printf("  %d/tcp open\n", ports[i]);
            found++;
        }
    }
    
    if (!found) printf("  No open ports found\n");
}

void cmd_online() {
    printf("Checking internet connectivity...\n\n");
    
    const char* hosts[] = {"1.1.1.1", "8.8.8.8", "google.com"};
    
    for (int i = 0; i < 3; i++) {
        printf("Testing %s... ", hosts[i]);
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ping -n 1 -w 1000 %s > nul 2>&1 && echo OK || echo FAILED", hosts[i]);
        system(cmd);
    }
}

void cmd_speed() {
    printf("Speed test using download...\n");
    
    HINTERNET hInternet = InternetOpenA("netool", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Error: Could not initialize\n");
        return;
    }
    
    HINTERNET hUrl = InternetOpenUrlA(hInternet, "http://speedtest.tele2.net/1MB.zip", NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hUrl) {
        char buffer[8192];
        DWORD bytesRead;
        DWORD totalBytes = 0;
        DWORD startTime = GetTickCount();
        DWORD duration = 5000;
        
        printf("Downloading test file for 5 seconds...\n");
        
        while ((GetTickCount() - startTime) < duration) {
            if (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                totalBytes += bytesRead;
            } else {
                break;
            }
        }
        
        DWORD elapsed = GetTickCount() - startTime;
        if (elapsed > 0) {
            double speed = (double)totalBytes * 8 / (elapsed / 1000.0) / 1000000.0;
            printf("Approximate download speed: %.2f Mbps\n", speed);
        }
        
        InternetCloseHandle(hUrl);
    } else {
        printf("Error: Could not connect to speed test server\n");
    }
    
    InternetCloseHandle(hInternet);
}

void cmd_whois(char* domain) {
    printf("WHOIS lookup for %s...\n\n", domain);
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Error: Could not create socket\n");
        return;
    }
    
    struct sockaddr_in whoisServer;
    whoisServer.sin_family = AF_INET;
    whoisServer.sin_port = htons(43);
    inet_pton(AF_INET, "199.7.83.42", &whoisServer.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&whoisServer, sizeof(whoisServer)) == 0) {
        char query[256];
        snprintf(query, sizeof(query), "%s\r\n", domain);
        send(sock, query, strlen(query), 0);
        
        char buffer[4096];
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            printf("%s", buffer);
        }
    } else {
        printf("Error: Could not connect to WHOIS server\n");
    }
    
    closesocket(sock);
}

void cmd_route() {
    printf("Routing Table\n");
    printf("=============\n\n");
    system("route print");
}

void cmd_flushdns() {
    printf("Flushing DNS cache...\n");
    system("ipconfig /flushdns");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_header();
        show_usage();
        return 0;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "myip") == 0) {
        int details = (argc > 2 && strcmp(argv[2], "--details") == 0);
        cmd_myip(details);
    }
    else if (strcmp(command, "localip") == 0) {
        cmd_localip();
    }
    else if (strcmp(command, "iface") == 0) {
        cmd_iface();
    }
    else if (strcmp(command, "dns") == 0) {
        cmd_dns();
    }
    else if (strcmp(command, "renew") == 0) {
        cmd_renew();
    }
    else if (strcmp(command, "reconnect") == 0) {
        cmd_reconnect();
    }
    else if (strcmp(command, "ping") == 0) {
        if (argc < 3) {
            printf("Usage: netool ping <host> [-t]\n");
            return 1;
        }
        int continuous = (argc > 3 && strcmp(argv[2], "-t") == 0);
        char* host = continuous ? argv[3] : argv[2];
        cmd_ping(host, continuous);
    }
    else if (strcmp(command, "trace") == 0) {
        if (argc < 3) {
            printf("Usage: netool trace <host>\n");
            return 1;
        }
        cmd_trace(argv[2]);
    }
    else if (strcmp(command, "ports") == 0) {
        if (argc > 2 && strcmp(argv[2], "--open") == 0) {
            cmd_ports_open();
        } else {
            printf("Usage: netool ports --open\n");
        }
    }
    else if (strcmp(command, "scan") == 0) {
        if (argc < 3) {
            printf("Usage: netool scan <host>\n");
            return 1;
        }
        cmd_scan(argv[2]);
    }
    else if (strcmp(command, "online") == 0) {
        cmd_online();
    }
    else if (strcmp(command, "speed") == 0) {
        cmd_speed();
    }
    else if (strcmp(command, "whois") == 0) {
        if (argc < 3) {
            printf("Usage: netool whois <domain>\n");
            return 1;
        }
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        cmd_whois(argv[2]);
        WSACleanup();
    }
    else if (strcmp(command, "route") == 0) {
        cmd_route();
    }
    else if (strcmp(command, "flushdns") == 0) {
        cmd_flushdns();
    }
    else if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_header();
        show_usage();
    }
    else {
        printf("Unknown command: %s\n", command);
        printf("Use 'netool help' for usage information.\n");
        return 1;
    }
    
    return 0;
}
