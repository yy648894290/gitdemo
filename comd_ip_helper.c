
#include "comd_share.h"
#include "comd_ip_helper.h"

int IS_IPv4_ADDR ( char *v4_addr )
{
    struct in_addr ipv4;

    /* Empty */
    if ( NULL == v4_addr || 0 == strlen ( v4_addr ) )
        return 0;

    return inet_pton ( AF_INET, v4_addr, &ipv4 );
}

int IS_IPv6_ADDR ( char *v6_addr )
{
    struct in6_addr ipv6;

    /* Empty */
    if ( NULL == v6_addr || 0 == strlen ( v6_addr ) )
        return 0;

    return inet_pton ( AF_INET6, v6_addr, &ipv6 );
}

int create_IPv4_gateway ( char *v4_addr, char *gateway )
{
    char ipv4[4] = {0};

    if ( 1 != inet_pton ( AF_INET, v4_addr, &ipv4 ) )
        return -1;

    ipv4[3] = ~ipv4[3];

    inet_ntop ( AF_INET, &ipv4, gateway, 32 );

    return 0;
}

int create_IPv6_gateway ( char *v6_addr, char *gateway )
{
    struct in6_addr ipv6;

    if ( 1 != inet_pton ( AF_INET6, v6_addr, &ipv6 ) )
        return -1;

    ipv6.s6_addr[15] = ~ ( ipv6.s6_addr[15] );

    inet_ntop ( AF_INET6, &ipv6, gateway, 64 );

    return 0;
}

int configure_link ( int fd, struct ifreq *ifr, int up )
{
    int ret;

    /* Open socket */
    fd = socket ( PF_PACKET, SOCK_DGRAM, IPPROTO_IP );
    if ( fd < 0 )
        return -1;

    /* Get flags, OR up flag, and apply the flags*/
    ret = ioctl ( fd, SIOCGIFFLAGS, ifr );
    if ( ret < 0 )
        return -1;

    if ( up )
    {
        ifr->ifr_flags |= IFF_UP;
        ifr->ifr_flags |= IFF_RUNNING;
    }
    else
    {
        ifr->ifr_flags &= ~IFF_UP;
        ifr->ifr_flags &= ~IFF_RUNNING;
    }

    ret = ioctl ( fd, SIOCSIFFLAGS, ifr );
    if ( ret < 0 )
        return -1;

    return 0;
}

int configure_ipv4 ( int fd, struct ifreq *ifr, char *ip )
{
    struct sockaddr_in sin;
    int ret;

    memset ( &sin, 0, sizeof ( struct sockaddr ) );
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr ( ( const char * ) ip );
    memcpy ( &ifr->ifr_addr, &sin, sizeof ( struct sockaddr ) );

    ret = ioctl ( fd, SIOCSIFADDR, ifr );
    if ( ret < 0 )
        goto error;

    memset ( &sin, 0, sizeof ( struct sockaddr ) );
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr ( ( const char * ) "255.255.255.0" );
    memcpy ( &ifr->ifr_addr, &sin, sizeof ( struct sockaddr ) );

    ret = ioctl ( fd, SIOCSIFNETMASK, ifr );

error:
    return ret;
}

int configure_ipv4_gateway ( int fd, struct ifreq *ifr, char *gateways )
{
    struct sockaddr_in sin;
    struct rtentry rt;

    memset ( &rt, 0, sizeof ( struct rtentry ) );
    memset ( &sin, 0, sizeof ( struct sockaddr_in ) );

    sin.sin_family = AF_INET;
    sin.sin_port = 0;

    sin.sin_addr.s_addr = inet_addr ( ( const char * ) gateways );
    memcpy ( &rt.rt_gateway, &sin, sizeof ( struct sockaddr_in ) );
    ( ( struct sockaddr_in * ) &rt.rt_dst )->sin_family = AF_INET;
    ( ( struct sockaddr_in * ) &rt.rt_genmask )->sin_family = AF_INET;
    rt.rt_flags = RTF_GATEWAY;

    if ( ioctl ( fd, SIOCADDRT, &rt ) < 0 )
        return -1;

    return 0;
}

int configure_ipv6 ( int fd, struct ifreq *ifr, char *ip )
{
    struct in6_ifreq ifr6;
    struct sockaddr_in6 sin;
    int ret;

    ret = ioctl ( fd, SIOCGIFINDEX, ifr );
    if ( ret < 0 )
        goto error;

    memset ( &sin, 0, sizeof ( struct sockaddr ) );
    sin.sin6_family = AF_INET6;
    if ( inet_pton ( AF_INET6, ip, ( void * ) &sin.sin6_addr ) <= 0 )
        goto error;

    memset ( &ifr6, 0, sizeof ( struct in6_ifreq ) );
    memcpy ( ( char * ) &ifr6.ifr6_addr, ( char * ) &sin.sin6_addr, sizeof ( struct in6_addr ) );
    ifr6.ifr6_ifindex = ifr->ifr_ifindex;
    ifr6.ifr6_prefixlen = 64;

    ret = ioctl ( fd, SIOCSIFADDR, &ifr6 );

error:
    return ret;
}

int configure_ipv6_gateway ( int fd, struct ifreq *ifr, char *gateways )
{

    return 0;
}

int net_address_fix ( char *ip, char *ip_buf )
{
    char *dotted_decimal;
    char *ret;
    char *ptr;
    char decimal[16];
    int i;

    /* Check if it is IPv4 */
    if ( IS_IPv4_ADDR ( ip ) )
    {
        strcpy ( ip_buf, ip );
        return 0;
    }

    /* Check if it is IPv6 */
    if ( IS_IPv6_ADDR ( ip ) )
    {
        strcpy ( ip_buf, ip );
        return 0;
    }

    if ( 0 == strlen ( ip ) )
        return 0;

    /* Assume dotted decimal IPv6 address */
    dotted_decimal = strdup ( ip );
    for ( ptr = dotted_decimal, i = 0; ptr != NULL; i++ )
    {
        ret = strsep ( &ptr, "." );
        if ( ret )
            decimal[i] = atoi ( ret );
    }

    if ( inet_ntop ( AF_INET6, decimal, ip_buf, 64 ) != NULL )
    {
        free ( dotted_decimal );
        return 0;
    }

    free ( dotted_decimal );
    return -1;
}

int net_config ( char *ifname, char *addresses, char *gateways, char *dnses )
{
    struct ifreq ifr;
    char *list;
    char *ip;
    char *ptr;
    int fd = -1;
    int fd6 = -1;
    int ret = -1;
    int ret4 = -1, ret6 = -1;

    CLOGD ( FINE, "IP Configuration: %s, %s, %s, %s\n", ifname, addresses, gateways, dnses );
    if ( !addresses || !gateways || !dnses )
    {
        CLOGD ( FINE, "IP Configuration error\n" );
        goto error;
    }

    /* Open socket */
    fd = socket ( AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if ( fd < 0 )
        goto error;

    fd6 = socket ( AF_INET6, SOCK_DGRAM, IPPROTO_IP );
    if ( fd < 0 )
        goto error;

    memset ( &ifr, 0, sizeof ( struct ifreq ) );
    strncpy ( ifr.ifr_name, ifname, IFNAMSIZ );

    /* Configure network interface to be up */
    ret = configure_link ( fd, &ifr, 1 );

    if ( ret ) goto error;

    /* Configure network addresses */
    list = strdup ( addresses );
    for ( ptr = list;; )
    {
        ip = strsep ( &ptr, " " );
        if ( ip == NULL )
            break;

        if ( IS_IPv4_ADDR ( ip ) )
            ret4 = configure_ipv4 ( fd, &ifr, ip );
        else if ( IS_IPv6_ADDR ( ip ) )
            ret6 = configure_ipv6 ( fd6, &ifr, ip );
    }
    free ( list );

    /* Configure gateway (or route) */
    configure_ipv4_gateway ( fd, &ifr, gateways );

    /* Configure dnses */

error:
    if ( fd > 0 )
        close ( fd );
    if ( fd6 > 0 )
        close ( fd6 );

    if ( !ret4 || !ret6 )
        return 0;
    else
        return -1;
}

int net_clear ( char *ifname )
{
    struct ifreq ifr;
    int fd = -1;
    int fd6 = -1;
    int ret = -1;

    /* Open socket */
    fd = socket ( AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if ( fd < 0 )
        goto error;

    fd6 = socket ( AF_INET6, SOCK_DGRAM, IPPROTO_IP );
    if ( fd < 0 )
        goto error;

    memset ( &ifr, 0, sizeof ( struct ifreq ) );
    strncpy ( ifr.ifr_name, ifname, IFNAMSIZ );

    /* Configure network interface to be down */
    ret = configure_link ( fd, &ifr, 0 );

    /* Configure dnses */

error:
    if ( fd > 0 )
        close ( fd );
    if ( fd6 > 0 )
        close ( fd6 );

    return ret;
}

unsigned int net_parse_str_by_separator ( unsigned char* input, unsigned char* output, int destLen, unsigned char separator )
{
    int i, j, k;
    int srcLen = 0;
    i = j = k = 0;

    if ( NULL == input || NULL == output || destLen == 0 )
    {
        return 0;
    }

    srcLen = strlen ( ( const char * ) input ) + 1; // which contains the '\0'
    if ( 1 == srcLen )
    {
        return 0;
    }

    for ( i = 0; i < srcLen; i++ )
    {
        if ( '\0' == input[i] )
        {
            if ( k <= ( destLen - 1 ) )
            {
                * ( output + destLen*j + k )  = '\0';
            }
            else
            {
                * ( output + destLen*j + ( destLen - 1 ) )  = '\0';
            }

            return j + 1;
        }

        if ( separator == input[i] )
        {
            if ( k <= ( destLen - 1 ) )
            {
                * ( output + destLen*j + k )  = '\0';
            }
            else
            {
                * ( output + destLen*j + ( destLen - 1 ) )  = '\0';
            }

            if ( '\0' != input[i+1] )
            {
                k = 0;
                j++;
            }
        }
        else
        {
            if ( k < ( destLen - 1 ) )
            {
                * ( output + destLen*j + k )  = input[i];
                k++;
            }
        }
    }

    return j;
}

int net_getIfIsExist ( char *ifname)
{
    int sockfd;
    struct ifreq ifr;

    if ( strlen ( ifname ) >= IFNAMSIZ )
    {
        ERR( "Interface name(%s) is error.\n", ifname );
        return 0;
    }

    strcpy ( ifr.ifr_name, ifname );
    if ( ( sockfd = socket ( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        ERR( "socket error.\n" );
        return 0;
    }

    //Get inet addr,netmask
    if ( ioctl ( sockfd, SIOCGIFHWADDR, &ifr ) < 0 )
    {
        //ERR( "ioctl request %d(sockfd:%d) error.\n", request, sockfd );
        close ( sockfd );
        return 0;
    }
    close ( sockfd );
    return 1;
}

