#ifndef PORN91_H
#define PORN91_H
// #include "porn91.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <linux/in.h>

static uint64_t _linux_get_time_ms(void) {
    struct timeval tv = {0};
    uint64_t time_ms;

    gettimeofday(&tv, NULL);

    time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time_ms;
}

static uint64_t _linux_time_left(uint64_t t_end, uint64_t t_now) {
    uint64_t t_left;

    if (t_end > t_now) {
        t_left = t_end - t_now;
    } else {
        t_left = 0;
    }

    return t_left;
}

uintptr_t HAL_TCP_Establish(const char *host, uint16_t port) {
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = 0;
    char service[6];


    uint8_t dns_retry = 0;

    memset(&hints, 0, sizeof(hints));

    LOGE("establish tcp connection with server(host='%s', port=[%u])\n", host, port);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%u", port);


    while (dns_retry++ < 8) {
        rc = getaddrinfo(host, service, &hints, &addrInfoList);
        if (rc != 0) {
            LOGE("getaddrinfo error[%d], res: %s, host: %s, port: %s\n", dns_retry,
                 gai_strerror(rc), host, service);
            sleep(1);
            continue;
        } else {
            break;
        }
    }

    if (rc != 0) {
        LOGE("getaddrinfo error(%d), host = '%s', port = [%d]\n", rc, host, port);
        return (uintptr_t) (-1);
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            LOGE("socket type error\n");
            rc = -1;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            LOGE("create socket error\n");
            rc = -1;
            continue;
        }

        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        close(fd);
        LOGE("connect error\n");
        rc = -1;
    }

    if (-1 == rc) {
        LOGE("fail to establish tcp\n");
    } else {
        LOGE("success to establish tcp, fd=%d\n", rc);
    }
    freeaddrinfo(addrInfoList);

    return (uintptr_t) rc;
}

int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms) {
    int ret, tcp_fd;
    uint32_t len_sent;
    uint64_t t_end, t_left;
    fd_set sets;
    int net_err = 0;

    t_end = _linux_get_time_ms() + timeout_ms;
    len_sent = 0;
    ret = 1; /* send one time if timeout_ms is value 0 */

    if (fd >= FD_SETSIZE) {
        return -1;
    }
    tcp_fd = (int) fd;

    do {
        t_left = _linux_time_left(t_end, _linux_get_time_ms());

        if (0 != t_left) {
            struct timeval timeout;

            FD_ZERO(&sets);
            FD_SET(tcp_fd, &sets);

            timeout.tv_sec = t_left / 1000;
            timeout.tv_usec = (t_left % 1000) * 1000;

            ret = select(tcp_fd + 1, NULL, &sets, NULL, &timeout);
            if (ret > 0) {
                if (0 == FD_ISSET(tcp_fd, &sets)) {
                    LOGE("Should NOT arrive\n");
                    /* If timeout in next loop, it will not sent any data */
                    ret = 0;
                    continue;
                }
            } else if (0 == ret) {
                LOGE("select-write timeout %d\n", tcp_fd);
                break;
            } else {
                if (EINTR == errno) {
                    LOGE("EINTR be caught\n");
                    continue;
                }

                LOGE("select-write fail, ret = select() = %d\n", ret);
                net_err = 1;
                break;
            }
        }

        if (ret > 0) {
            ret = send(tcp_fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                LOGE("No data be sent\n");
            } else {
                if (EINTR == errno) {
                    LOGE("EINTR be caught\n");
                    continue;
                }

                LOGE("send fail, ret = send() = %d\n", ret);
                net_err = 1;
                break;
            }
        }
    } while (!net_err && (len_sent < len) && (_linux_time_left(t_end, _linux_get_time_ms()) > 0));

    if (net_err) {
        return -1;
    } else {
        return len_sent;
    }
}

int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms) {
    int ret, err_code, tcp_fd;
    uint32_t len_recv;
    uint64_t t_end, t_left;
    fd_set sets;
    struct timeval timeout;

    t_end = _linux_get_time_ms() + timeout_ms;
    len_recv = 0;
    err_code = 0;

    if (fd >= FD_SETSIZE) {
        return -1;
    }
    tcp_fd = (int) fd;

    do {
        t_left = _linux_time_left(t_end, _linux_get_time_ms());
        if (0 == t_left) {
            break;
        }
        FD_ZERO(&sets);
        FD_SET(tcp_fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        ret = select(tcp_fd + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            ret = recv(tcp_fd, buf + len_recv, len - len_recv, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                printf("connection is closed\n");
                err_code = -1;
                break;
            } else {
                if (EINTR == errno) {
                    continue;
                }
                printf("recv fail\n");
                err_code = -2;
                break;
            }
        } else if (0 == ret) {
            break;
        } else {
            if (EINTR == errno) {
                continue;
            }
            printf("select-recv fail\n");
            err_code = -2;
            break;
        }
    } while ((len_recv < len));

    /* priority to return data bytes if any data be received from TCP connection. */
    /* It will get error code on next calling */
    return (0 != len_recv) ? len_recv : err_code;
}

int get91PornVideo(request, request_len) {
    uintptr_t rc = HAL_TCP_Establish("91porn.com", 80);
    int32_t ret = HAL_TCP_Write(rc, request, request_len, 5000);
    char buf[1024];
    char rsp[32768];
    rsp[0] = 0;
    char *body;
    int found = 0;
    int total = 0;
    while (1) {
        int len = HAL_TCP_Read(rc, buf, 1024, 3000);
        if (len < 0) {
            break;
        }
        total += len;
        if (total >= 32767) {
            break;
        }
        strncat(rsp, buf, len);
        if (!found && (body = strstr(rsp, "\r\n\r\n"))) {
            found = 1;
            LOGD("%s", body);
        }
        if (strstr(rsp, "\r\n0\r\n")) {
            LOGE("%s", "===============");
            break;
        }
    }

    return 0;
}

// https://github.com/Tencent/TencentOS-tiny/blob/master/components/connectivity/iotkit-embedded-3.0.1/3rdparty/wrappers/os/ubuntu/HAL_TCP_linux.c

#endif
