
#ifndef _DSPTTK_INET_H_
#define _DSPTTK_INET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include "sal_type.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN  16     // for IPv4 dotted-decimal
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46     // for IPv6 hex string
#endif

/**
 * struct sockaddr是通用的套接字地址，而struct sockaddr_in则是internet环境下套接字的地址形式。
 * 二者长度一样，都是16个字节；二者是并列结构，指向sockaddr_in结构的指针也可以指向sockaddr。
 * 一般情况下，需要把sockaddr_in结构强制转换成sockaddr结构再传入系统调用函数中。
 */

/**
 * 结构体struct sockaddr在/usr/include/linux/socket.h中定义：
 * typedef unsigned short sa_family_t;
 * struct sockaddr
 * {
 *     sa_family_t sa_family;   // address family, AF_xxx
 *     char        sa_data[14]; // 14 bytes of protocol address
 * };
 */

/**
 * 结构体struct sockaddr_in在/usr/include/netinet/in.h中定义：
 * Structure describing an Internet socket address.
 *  
 * typedef uint16_t in_port_t;
 * typedef uint32_t in_addr_t;
 *  
 * struct in_addr {
 *     in_addr_t s_addr; // 'struct in_addr'其实就是32位IP地址
 * };
 *  
 * struct sockaddr_in
 * {
 *     unsigned short sin_family;
 *     in_port_t      sin_port; // Port number
 *     struct in_addr sin_addr; // Internet address
 *     // Pad to size of 'struct sockaddr'
 *     // 字符数组sin_zero[8]的存在是为了保证结构体struct sockaddr_in和结构体struct sockaddr的大小相等
 *     unsigned char  sin_zero[sizeof(struct sockaddr) - sizeof(unsigned short) - sizeof(in_port_t) - sizeof(struct in_addr)];
 * }
 * 
 * struct in6_addr {
 *     unsigned char   s6_addr[16];   // IPv6 address
 * };
 * 
 * struct sockaddr_in6 {
 *     sa_family_t     sin6_family;   // AF_INET6
 *     in_port_t       sin6_port;     // port number
 *     uint32_t        sin6_flowinfo; // IPv6 flow information
 *     struct in6_addr sin6_addr;     // IPv6 address
 *     uint32_t        sin6_scope_id; // Scope ID (new in 2.4)
 * }; 
 */

#if 0
typedef struct
{
	union
	{
		struct sockaddr_in  _sin4;	/**< IPV4 地址*/
		struct sockaddr_in6 _sin6;	/**< IPV6 地址*/
	}_sa;
}SOCKET_ADDR_T;
#endif

typedef union sock_addr_u
{
	struct sockaddr_in  sin4;	// IPv4 socket 地址
	struct sockaddr_in6 sin6;	// IPv6 socket 地址
}SOCK_ADDR;

typedef union in_addr_u
{
	struct in_addr	ipv4;		// IPv4地址，4个字节
	struct in6_addr	ipv6;		// IPv6地址，16个字节
}IN_ADDR;

/**
 * @fn      dspttk_socket_create
 * @brief   即封装socket函数：int socket(int af_type, int sock_type, int protocol)
 *
 * @param   [IN] af_type 协议族：AF_INET、AF_INET6、AF_LOCAL/AF_UNIX、AF_ROUTE等，
                         它决定了socket的地址类型，在通信中必须采用对应的地址，
                         如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合，
                         AF_UNIX决定了要用一个绝对路径名作为地址。
 * @param   [IN] sock_type socket类型：SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_NONBLOCK等
 * @param   [IN] protocol 指定协议，常用的协议有：IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等。
 *                        并不是type和protocol可以随意组合的，如SOCK_STREAM不可以跟IPPROTO_UDP组合。
 *                       当protocol为0时，会自动选择type类型对应的默认协议。
 *
 * @return  On success, a file descriptor for the new socket is returned. On error, -1 is returned.
 */
INT32 dspttk_socket_create(INT32 af_type, INT32 sock_type);

/**
 * @fn      dspttk_socket_bind
 * @brief   封装bind函数：int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [IN] p_sock_addr IPV4&IPV6套接字地址形式
 *
 * @return  SAL_SOK: bind success, SAL_FAIL: bind failed
 */
SAL_STATUS dspttk_socket_bind(INT32 sockfd, SOCK_ADDR *p_sock_addr);

/**
 * @fn      dspttk_socket_listen
 * @brief   封装listen函数：int listen(int sockfd, int backlog);
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [IN] connections_max 在套接字上排队的最大连接数
 *
 * @return  SAL_SOK: listen success, SAL_FAIL: listen failed
 */
SAL_STATUS dspttk_socket_listen(INT32 sockfd, INT32 connections_max);

/**
 * @fn      dspttk_socket_connect
 * @brief   封装connect函数：int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 * 
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [IN] sock_addr 服务器地址信息，注：其中参数需转为网络字节序
 * @param   [IN] timeout_ms 连接超时时间，SAL_TIMEOUT_FOREVER为永久等待
 * 
 * @return  SAL_SOK: connect success, SAL_FAIL: connect failed
 */
SAL_STATUS dspttk_socket_connect(INT32 sockfd, SOCK_ADDR *sock_addr, UINT32 timeout_ms);

/**
 * @fn      dspttk_socket_accept
 * @brief   封装accept函数：int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [OUT] p_client_addr 对端地址信息，如果对地址不感兴趣，可以置为NULL
 *          This structure is filled in with the address of the peer socket
 *
 * @return  On success, a nonnegative integer(connfd) that is a descriptor for the accepted socket is returned.
 *          On error, -1 is returned, and errno is set appropriately.
 */
INT32 dspttk_socket_accept(INT32 sockfd, SOCK_ADDR *p_client_addr);

/**
 * @fn      dspttk_socket_recv
 * @brief   封装recv函数：ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 *          只要接收到数据，该接口即返回
 *
 * @param   [IN] connfd client端为sockfd，server端为accept返回的connfd
 * @param   [IN] buf 接收数据的缓冲区地址
 * @param   [IN] buf_len 缓冲区大小
 * @param   [IN] timeout_ms 接收超时时间，SAL_TIMEOUT_FOREVER为永久等待
 * 
 * @return  在timeout_ms时间实际接收到的数据长度，失败时返回-1
 */
INT32 dspttk_socket_recv(INT32 connfd, void *buf, size_t buf_len, UINT32 timeout_ms);

/**
 * @fn      dspttk_socket_send
 * @brief   封装send函数：ssize_t send(int sockfd, const void *buf, size_t len, int flags);
 * 
 * @param   [IN] connfd client端为sockfd，server端为accept返回的connfd
 * @param   [IN] buf 发送数据的缓冲区地址
 * @param   [IN] count 需要发送数据的长度
 * @param   [IN] timeout_ms 发送超时时间，SAL_TIMEOUT_FOREVER为永久等待
 * 
 * @return  在timeout_ms时间实际发送的数据长度，失败时返回-1
 */
INT32 dspttk_socket_send(INT32 connfd, const void *buf, size_t count, UINT32 timeout_ms);

/**
 * @fn      dspttk_socket_ling
 * @brief   设置closed TCP socket的行为，是否发送缓冲区中残留数据
 * 
 * @param   [IN] sockfd client端为sockfd，server端为accept返回的connfd
 * @param   [IN] b_enable TRUE：使能，继续发送缓冲区中的残留数据，FALSE：丢弃缓冲区中的残留数据，直接结束连接
 * @param   [IN] timeout_sec 延迟时间，单位：秒
 */
void dspttk_socket_ling(INT32 sockfd, BOOL b_enable, UINT32 timeout_sec);

/**
 * @fn      dspttk_socket_reuse
 * @brief   重置socket的本地地址和端口号，使立即可以被再次绑定
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 *
 * @return  SAL_SOK: set reuse success, SAL_FAIL: set reuse failed
 */
SAL_STATUS dspttk_socket_reuse(INT32 sockfd);

/**
 * 点分十进制转网络字节序二进制值，返回0表示转换失败，点分十进制地址非法
 * > int inet_aton(const char *cp, struct in_addr *inp);
 *
 * 点分十进制转网络字节序二进制值，与inet_aton不同在于，它是作为返回值输出
 * > in_addr_t inet_addr(const char *cp); // 避免使用
 *
 * 点分十进制转主机字节序二进制值，若输入IP地址不正确，返回值是-1(255.255.255.255)
 * > in_addr_t inet_network(const char *cp);
 *
 * 网络字节序二进制转点分十进制IPV4地址。
 * 需要注意的是inet_ntoa函数的返回值直到下次调用前一直有效，
 * 所以如果在线程中使用inet_ntoa的时候，一定要确保每次只有一个线程调用本函数，
 * 否则一个线程的返回的结构可能被其他线程返回的结果所覆盖。
 * > char *inet_ntoa(struct in_addr in);
 *
 * IP地址（网络字节序）转换为没有网络位的主机ID（主机字节序）：
 * A类地址的后三段，B类地址的后两段，C类地址的最后一段
 * > in_addr_t inet_lnaof(struct in_addr in);
 *
 * IP地址（网络字节序）转换为网络ID（主机字节序）：
 * A类地址的第一段，B类地址的前两段，C类地址的前三段
 * > in_addr_t inet_netof(struct in_addr in);
 *
 * 将网络位和主机ID合并为一个新的IP地址
 * > struct in_addr inet_makeaddr(int net, int host);
 *
 * #include <sys/socket.h>
 * #include <netinet/in.h>
 * #include <arpa/inet.h>
 *
 * > inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation
 * into binary form in network byte order and stores it in the structure that inp points to.
 * inet_aton() returns nonzero if the address is valid, zero if not.
 *
 * > inet_addr() function converts the Internet host address cp from IPv4 numbers-and-dots notation
 * into binary data in network byte order. If the input is invalid, INADDR_NONE(usually -1) is returned.
 * Use of this function is problematic because -1 is a valid address(255.255.255.255).
 * Avoid its use in favor of inet_aton(), inet_pton(3), or getaddrinfo(3)
 * which provide a cleaner way to indicate error return.
 *
 * > inet_network() function converts cp, a string in IPv4 numbers-and-dots notation,
 * into a number in host byte order suitable for use as an Internet network address.
 * On success, the converted address is returned. If the input is invalid, -1 is returned.
 *
 * > inet_ntoa() function converts the Internet host address in, given in network byte order,
 * to a string in IPv4 dotted-decimal notation.
 * The string is returned in a statically allocated buffer, which subsequent calls will overwrite.
 *
 * > inet_lnaof() function returns the local network address part of the Internet address in.
 * The returned value is in host byte order.
 *
 * > inet_netof() function returns the network number part of the Internet address in.
 * The returned value is in host byte order.
 *
 * > inet_makeaddr() function is the converse of inet_netof() and inet_lnaof().
 * It returns an Internet host address in network byte order,
 * created by combining the network number net with the local address host, both in host byte order.
 */


#ifdef __cplusplus
}
#endif

#endif

