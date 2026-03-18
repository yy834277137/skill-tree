
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include "prt_trace.h"
#include "sal_macro.h"
#include "dspttk_util.h"
#include "dspttk_inet.h"


#if 0
#ifndef IN6ADDRSZ
#define IN6ADDRSZ   16   /* IPv6 T_AAAA */
#endif
#ifndef IN4ADDRSZ
#define IN4ADDRSZ   4   /* IPv6 T_AAAA */
#endif

#ifndef INT16SZ
#define INT16SZ     2    /* word size */
#endif

#define LOCAL_INTERFACE		"eth0"
#define LOOP_INTERFACE		"lo"
#define PPP_INTERFACE		"ppp0"
#define IP_NULL				"0.0.0.0"
// INADDR_ANY就是指定地址为0.0.0.0的地址，这个地址事实上表示不确定地址，或“所有地址”、“任意地址”

#define MAX_IP_STRING_LEN 128 ///< 字符串形式ip地址的最大长度
#define TIME_OUT_INFINITE 0xFFFFFFFF ///< connct时用不超时，即阻塞调用
#endif


//extern const struct in6_addr g_any6addr; ///< 表示不绑定任何地址


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
INT32 dspttk_socket_create(INT32 af_type, INT32 sock_type)
{
	INT32 socket_fd = -1;

	socket_fd = socket(af_type, sock_type, 0);

	if (socket_fd < 0)
	{
		PRT_INFO("socket create failed, errno: %d, %s\n", errno, strerror(errno));
	}

	return socket_fd;
}


/**
 * @fn      dspttk_socket_bind
 * @brief   封装bind函数：int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [IN] p_sock_addr IPV4&IPV6套接字地址形式
 *
 * @return  SAL_SOK: bind success, SAL_FAIL: bind failed
 */
SAL_STATUS dspttk_socket_bind(INT32 sockfd, SOCK_ADDR *p_sock_addr)
{
	INT32 ret_val = SAL_SOK;

	if (NULL == p_sock_addr)
	{
		PRT_INFO("p_sock_addr is NULL\n");
		return SAL_FAIL;
	}

	if (p_sock_addr->sin4.sin_family == AF_INET)
	{
		ret_val = bind(sockfd, (struct sockaddr *)&p_sock_addr->sin4, sizeof(struct sockaddr_in));
		if (ret_val < 0)
		{
			ret_val = SAL_FAIL;
			PRT_INFO("AF_INET, socket bind failed, sockfd: %d, errno: %d, %s\n", sockfd, errno, strerror(errno));
		}
		else
		{
			ret_val = SAL_SOK;
		}
	}
	else if (p_sock_addr->sin6.sin6_family == AF_INET6)
	{
		ret_val = bind(sockfd, (struct sockaddr *)&p_sock_addr->sin6, sizeof(struct sockaddr_in6));
		if (ret_val < 0)
		{
			ret_val = SAL_FAIL;
			PRT_INFO("AF_INET6, socket bind failed, sockfd: %d, errno: %d, %s\n", sockfd, errno, strerror(errno));
		}
		else
		{
			ret_val = SAL_SOK;
		}
	}
	else
	{
		ret_val = SAL_FAIL;
		PRT_INFO("socket family[%d] is neither AF_INET nor AF_INET6\n", p_sock_addr->sin4.sin_family);
	}

	return ret_val;
}


/**
 * @fn      dspttk_socket_listen
 * @brief   封装listen函数：int listen(int sockfd, int backlog);
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 * @param   [IN] connections_max 在套接字上排队的最大连接数
 *
 * @return  SAL_SOK: listen success, SAL_FAIL: listen failed
 */
SAL_STATUS dspttk_socket_listen(INT32 sockfd, INT32 connections_max)
{
	INT32 ret_val = 0;

	ret_val = listen(sockfd, connections_max);
	if (ret_val < 0)
	{
		PRT_INFO("socket listen failed, sockfd: %d, backlog: %d, errno: %d, %s\n", \
				sockfd, connections_max, errno, strerror(errno));
		ret_val = SAL_FAIL;
	}
	else
	{
		ret_val = SAL_SOK;
	}

	return ret_val;
}


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
SAL_STATUS dspttk_socket_connect(INT32 sockfd, SOCK_ADDR *sock_addr, UINT32 timeout_ms)
{
	INT32 ret_val = 0;
	struct timeval timeout = {0};
	socklen_t addrlen = 0;
	CHAR svr_addr[INET6_ADDRSTRLEN] = {0};
	UINT16 svr_port = 0;

	if (NULL == sock_addr)
	{
		PRT_INFO("sock_addr is NULL\n");
		return SAL_FAIL;
	}

	if (timeout_ms > 0)
	{
		timeout.tv_sec = timeout_ms / 1000;
		timeout.tv_usec = (timeout_ms % 1000) * 1000;
		ret_val = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
		if (0 != ret_val)
		{
			PRT_INFO("set SO_SNDTIMEO failed, errno: %d, %s\n", errno, strerror(errno));
			return SAL_FAIL;
		}
	}

	if (sock_addr->sin4.sin_family == AF_INET6)
	{
		addrlen = sizeof(sock_addr->sin6);
		inet_ntop(AF_INET6, &sock_addr->sin6.sin6_addr, svr_addr, sizeof(svr_addr));
		svr_port = ntohs(sock_addr->sin6.sin6_port);
	}
	else
	{
		addrlen = sizeof(sock_addr->sin4);
		inet_ntop(AF_INET, &sock_addr->sin4.sin_addr, svr_addr, sizeof(svr_addr));
		svr_port = ntohs(sock_addr->sin4.sin_port);
	}

	/**
	 * When you call connect() on a non-blocking socket, you'll get 
	 * EINPROGRESS instead of blocking waiting for the connection 
	 * handshake to complete. Then, you have to select() for 
	 * writability, and check the socket error to see if the 
	 * connection has completed. 
	 */
	ret_val = connect(sockfd, (struct sockaddr *)sock_addr, addrlen);
	if (ret_val == 0)
	{
		PRT_INFO("connect success, addr info: %s @ %d\n", svr_addr, svr_port);
		ret_val = SAL_SOK;
	}
	else
	{
		PRT_INFO("connect faild, %ums, addr info: %s @ %d, errno: %d, %s\n", \
				timeout_ms, svr_addr, svr_port, errno, strerror(errno));
		ret_val = SAL_FAIL;
	}

	return ret_val;
}



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
INT32 dspttk_socket_accept(INT32 sockfd, SOCK_ADDR *p_client_addr)
{
	INT32 connfd = -1;
	socklen_t addr_size = 0;

	if (NULL == p_client_addr)
	{
		connfd = accept(sockfd, NULL, NULL);
	}
	else
	{
		if (p_client_addr->sin4.sin_family == AF_INET6)
		{
			addr_size = sizeof(p_client_addr->sin6);
		}
		else
		{
			addr_size = sizeof(p_client_addr->sin4);
		}
		connfd = accept(sockfd, (struct sockaddr *)p_client_addr, &addr_size);
	}

	if (connfd < 0)
	{
		PRT_INFO("socket accept failed, sockfd: %d, errno: %d, %s\n", sockfd, errno, strerror(errno));
	}

	return connfd;
}


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
INT32 dspttk_socket_recv(INT32 connfd, void *buf, size_t buf_len, UINT32 timeout_ms)
{
	INT32 nrecv = 0, ret_val = 0;
	struct timeval timeout = {0};
	fd_set rset;

	if (NULL == buf || buf_len == 0 || connfd < 0)
	{
		PRT_INFO("buf[%p] OR buf_len[%zu] OR connfd[%d] is invalid\n", buf, buf_len, connfd);
		return -1;
	}

	FD_ZERO(&rset);
	FD_SET(connfd, &rset);

	// recv超时可使用setsockopt的SO_RCVTIMEO选项
	if (SAL_TIMEOUT_FOREVER == timeout_ms)
	{
		ret_val = select(connfd + 1, &rset, NULL, NULL, NULL); /* wait forever */
	}
	else
	{
		timeout.tv_sec = timeout_ms / 1000;
		timeout.tv_usec = (timeout_ms % 1000) * 1000;
		ret_val = select(connfd + 1, &rset, NULL, NULL, &timeout);
	}

	if (ret_val > 0 && FD_ISSET(connfd, &rset))
	{
		while (1)
		{
			nrecv = recv(connfd, buf, buf_len, 0);
			if (nrecv > 0)
			{
				//PRT_INFO("receive the number of bytes: %d, buf_len: %zu\n", nrecv, buf_len);
				break;
			}
			else if (0 == nrecv)
			{
				PRT_INFO("Maybe the peer has performed an orderly shutdown\n");
				nrecv = -1;
				break;
			}
			else if (nrecv < 0)
			{
				if (errno == EINTR)
				{
					PRT_INFO("EINTR accurred, try again\n");
				}
				else
				{
					PRT_INFO("recv failed, ret_val: %d, buf_len: %zu, errno: %d, %s\n", nrecv, buf_len, errno, strerror(errno));
					break;
				}
			}
		}
	}
	else if (0 == ret_val)
	{
		//PRT_INFO("selcet timeout...\n");
		nrecv = 0;
	}
	else
	{
		/* if return 0, selcet timeout, else return -1 */
		PRT_INFO("select failed, ret_val: %d, errno: %d, %s\n", ret_val, errno, strerror(errno));
		nrecv = -1;
	}

	return nrecv;
}


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
INT32 dspttk_socket_send(INT32 connfd, const void *buf, size_t count, UINT32 timeout_ms)
{
	UINT32 nleft = count;
	CHAR *ptr = (char *)buf;
	INT32 ret_val = 0, nsend = 0;
	fd_set wset;
	struct timeval wait_time = {0};

	if (NULL == buf || count == 0 || connfd < 0)
	{
		PRT_INFO("buf[%p] OR count[%zu] OR connfd[%d] is invalid\n", buf, count, connfd);
		return -1;
	}

	if (SAL_TIMEOUT_FOREVER != timeout_ms)
	{
		wait_time.tv_sec = timeout_ms / 1000;
		wait_time.tv_usec = (timeout_ms % 1000) * 1000;
	}

	FD_ZERO(&wset);
	FD_SET(connfd, &wset);
	if (SAL_TIMEOUT_FOREVER == timeout_ms)
	{
		ret_val = select(connfd+1, NULL, &wset, NULL, NULL); /* wait forever */
	}
	else
	{
		ret_val = select(connfd+1, NULL, &wset, NULL, &wait_time);
	}

	if (ret_val > 0)
	{
		if (FD_ISSET(connfd, &wset))
		{
			while (nleft > 0)
			{
				/** 
				 * 当连接断开，继续发数据的时候，不仅send()的返回值会有指示，而且还会像系统发送一个异常消息， 
				 * 如果不作处理，系统会出BrokePipe，程序会退出。 
				 * 为此，send()函数的最后一个参数可以设MSG_NOSIGNAL，禁止send()函数向系统发送异常消息 
				 */
				nsend = send(connfd, ptr, nleft, MSG_NOSIGNAL);
				if (-1 == nsend)
				{
					if (errno == EINTR)
					{
						PRT_INFO("EINTR accurred, try to send again\n");
						continue;
					}
					else
					{
						PRT_INFO("send failed, errno: %d, %s\n", errno, strerror(errno));
						break;
					}
				}

				nleft -= nsend;
				ptr   += nsend;
			}
		}
	}
	else if (0 == ret_val)
	{
		PRT_INFO("select timeout...\n");
	}
	else
	{
		/* if selcet timeout, return 0, else return -1 */
		PRT_INFO("select failed, ret_val: %d, errno: %d, %s\n", ret_val, errno, strerror(errno));
	}

	return (count-nleft);
}


/**
 * @fn      dspttk_socket_ling
 * @brief   设置closed TCP socket的行为，是否发送缓冲区中残留数据
 * 
 * @param   [IN] sockfd client端为sockfd，server端为accept返回的connfd
 * @param   [IN] b_enable TRUE：使能，继续发送缓冲区中的残留数据，FALSE：丢弃缓冲区中的残留数据，直接结束连接
 * @param   [IN] timeout_sec 延迟时间，单位：秒
 */
void dspttk_socket_ling(INT32 sockfd, BOOL b_enable, UINT32 timeout_sec)
{
	INT32 ret_val = 0;
	socklen_t optlen = sizeof(struct linger);

	struct linger ling = {
		.l_onoff = b_enable,		/* linger active */
		.l_linger = timeout_sec,	/* how many seconds to linger for */
	};

	ret_val = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger));
	if (-1 == ret_val)
	{
		PRT_INFO("set SO_LINGER failed, errno: %d, %s\n", errno, strerror(errno));
		ret_val = getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void *)&ling, &optlen);
		if (0 == ret_val)
		{
			PRT_INFO("leave the SO_LINGER as it is, onoff: %d, ling_time: %ds\n", ling.l_onoff, ling.l_linger);
		}
	}

	return;
}


/**
 * @fn      dspttk_socket_reuse
 * @brief   重置socket的本地地址和端口号，使立即可以被再次绑定
 *
 * @param   [IN] sockfd socket句柄，由dspttk_socket_create函数创建
 *
 * @return  SAL_SOK: set reuse success, SAL_FAIL: set reuse failed
 */
SAL_STATUS dspttk_socket_reuse(INT32 sockfd)
{
	INT32 ret_val = -1;
	BOOL tmp = SAL_TRUE;

	ret_val = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
	if (ret_val < 0)
	{
		PRT_INFO("set SO_REUSEADDR failed, errno: %d, %s\n", errno, strerror(errno));
	}

	return ret_val;
}

