/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2009, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  sysdep_w32.h: system dependent support for WIN32 environments
 *
 *  Known contributors to this file:
 *     Ron Koenderink, 2007
 *     Markus Armbruster, 2009
 */

#ifndef _SYSDEF_W32_H
#include <getopt.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#ifdef _MSC_VER
typedef int __w64 ssize_t;
#endif

struct passwd {
    char *pw_name;
};

struct iovec {
    void *iov_base;
    size_t iov_len;
};

#define W32_FD_TO_SOCKET(fd) ((SOCKET)_get_osfhandle((fd)))
#define W32_SOCKET_TO_FD(fh) (_open_osfhandle((long)(fh), O_RDWR | O_BINARY))

extern void w32_set_winsock_errno(void);

extern int w32_recv(int sockfd, void *, size_t, int flags);
extern int w32_send(int sockfd, const void *, size_t, int flags);
extern int w32_close(int fd);
extern int w32_socket(int domain, int type, int protocol);
extern int w32_connect(int sockfd, const struct sockaddr *, int addrlen);
extern ssize_t w32_readv_fd(int fd, const struct iovec *iov,
			    int iovcnt);
extern ssize_t w32_writev_socket(int sockfd, const struct iovec *iov,
				 int iovcnt);

extern struct passwd *w32_getpw(void);
extern void w32_sysdep_init(void);

#define recv(sockfd, buffer, buf_size, flags) \
    w32_recv((sockfd), (buffer), (buf_size), (flags))
#define close(fd) \
    w32_close((fd))
#define socket(domain, type, protocol) \
    w32_socket((domain), (type), (protocol))
#define connect(sockfd, addr, addrlen) \
    w32_connect((sockfd), (addr), (addrlen))

#ifdef _MSC_VER
#define pclose _pclose
#define popen _popen
#define snprintf _snprintf
#endif
#endif
