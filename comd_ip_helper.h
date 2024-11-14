
#ifndef _COMD_IP_HELPER_H
#define _COMD_IP_HELPER_H 1

#define USE_DEFAULT_IPV6_LINK_LOCAL 1

int IS_IPv4_ADDR ( char *v4_addr );
int IS_IPv6_ADDR ( char *v6_addr );
int create_IPv4_gateway ( char *v4_addr, char *gateway );
int create_IPv6_gateway ( char *v6_addr, char *gateway );
int configure_link ( int fd, struct ifreq *ifr, int up );
int configure_ipv4 ( int fd, struct ifreq *ifr, char *ip );
int configure_ipv4_gateway ( int fd, struct ifreq *ifr, char *gateways );
int configure_ipv6 ( int fd, struct ifreq *ifr, char *ip );
int configure_ipv6_gateway ( int fd, struct ifreq *ifr, char *gateways );

int net_address_fix ( char *ip, char *ip_buf );
int net_config ( char *ifname, char *addresses, char *gateways, char *dnses );
int net_clear ( char *ifname );
unsigned int net_parse_str_by_separator ( unsigned char* input, unsigned char* output, int destLen, unsigned char separator );
int net_getIfIsExist ( char *ifname);

#endif /* _COMD_IP_HELPER_H */
