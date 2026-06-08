netool - Network Utility Tool
Developers: ExEintel
================================

DESCRIPTION
-----------
netool is a command-line network utility for Windows. It provides tools for 
network diagnostics, connection management, and internet-related technologies.

REQUIREMENTS
------------
- Windows OS (7/8/10/11)
- MinGW/gcc (for building from source)
- Internet connection (for public IP detection)

BUILDING
--------
gcc main.c -o netool.exe -lwininet -liphlpapi -lws2_32

Or simply compile with:
gcc main.c -o netool.exe

COMMANDS
--------

1. BASIC DIAGNOSTICS

netool myip
    Show your public IPv4 address.
    Example: IPv4: 79.139.135.254

netool myip --details
    Show public IP with provider, city, country.
    Example:
        IPv4: 79.139.135.254 (Provider: ISP, City: Moscow, Country: Russia)

netool localip
    Show all local IP addresses of network interfaces.
    Example:
        Ethernet: 192.168.1.5
        WiFi: 192.168.1.100

netool iface
    List network interfaces and their state (up/down).
    Example:
        1. Ethernet (Ethernet) - up
        2. WiFi (IEEE 802.11) - down

netool dns
    Show current DNS servers used by the system.

2. CONNECTION MANAGEMENT

netool renew
    Renew DHCP lease (request new IP from router).
    Note: Uses Windows ipconfig /renew command.

netool reconnect
    Reconnect active interface (release and renew).
    Note: Requires administrator privileges.

3. PING AND CONNECTIVITY

netool ping <host>
    Ping host with 4 packets.
    Example: netool ping google.com

netool ping -t <host>
    Continuous ping until Ctrl+C.
    Example: netool ping -t 8.8.8.8

netool trace <host>
    Traceroute to host (shows network path).
    Example: netool trace ya.ru
    Note: Uses Windows tracert command.

4. PORTS AND SERVICES

netool ports --open
    Show open ports on your computer.
    Note: Uses netstat command.

netool scan <host>
    Quick scan of common ports (21, 22, 23, 25, 53, 80, 110, 143, 443, 445, 3389).
    Example: netool scan 192.168.1.1

5. CONNECTION CONTROL

netool online
    Check internet connectivity by pinging:
    - 1.1.1.1 (Cloudflare DNS)
    - 8.8.8.8 (Google DNS)  
    - google.com

netool speed
    Simple speed test using download from speedtest.tele2.net.
    Note: Tests download speed only for 5 seconds.

6. ADVANCED

netool whois <domain>
    Show WHOIS information for domain.
    Example: netool whois google.com
    Note: Connects to whois.iana.org (199.7.83.42).

netool route
    Show system routing table.
    Note: Uses Windows 'route print' command.

netool flushdns
    Flush DNS cache.
    Note: Uses Windows 'ipconfig /flushdns' command.

EXAMPLES
--------
netool myip
netool localip
netool ping google.com
netool trace 8.8.8.8
netool scan 192.168.1.1
netool online
netool whois example.com

NOTES
-----
- Some commands require administrator privileges
- Public IP detection requires internet connection
- Speed test is approximate (5-second download test)
- WHOIS uses IANA server (may refer to other servers)
- Some features use Windows built-in commands (ipconfig, tracert, netstat)

VERSION
-------
1.0 (2026)

AUTHORS
-------
ExEintel

LICENSE
-------
Free to use and modify for educational purposes.
THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
