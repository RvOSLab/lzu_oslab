#include <net/list.h>
#include <net/socket.h>
#include <net/netdef.h>
#include <kdebug.h>
#include <mm.h>
#include <utils/atomic.h>

static LIST_HEAD(sockets);
static int socket_count = 0;
struct spinlock slock;

void init_socket_lock()  
{  
    init_lock(&slock, "socket_lock");  
}  
 
static inline void socket_sockets_enqueue(struct socket *sk) {
    acquire_lock(&slock);
	list_add_tail(&sk->list, &sockets);
	socket_count++;
	release_lock(&slock);
}

static inline void socket_sockets_remove(struct socket *sk) {
	acquire_lock(&slock);
	list_del_init(&sk->list);
	socket_count--;
	release_lock(&slock);
}

extern struct net_family inet;

/* AF_INET=2 */
static struct net_family *families[128] = {
	/* 这里纯粹是为了保留unix函数的一点影子,实际上可以去除掉 */
	[AF_INET] = &inet,
};


static struct socket *alloc_socket(pid_t pid) {
	/* todo: 这里需要想出一种方法使得我们的fd不会和kernel的fd重叠,
	 所以这里的fd设置得非常大. */
	static int fd = 4097;
	struct socket *sock = kmalloc(sizeof(struct socket));
	list_init(&sock->list);

	sock->pid = pid;	/* 进程id */
	sock->fd = fd++;	/* 不重复文件描述符 */
	sock->ops = NULL;
	sock->flags = O_RDWR;
	return sock;
}

int free_socket(struct socket *sock) {
	if (!sock) return -1;

	if (sock->ops) {
		sock->ops->free(sock);
	}
	acquire_lock(&slock);
	list_del(&sock->list);
	/* 需要注意的是,这里并不删除struct socket中的struct sock,因为这个socket
	对应的sock可能还需要处理一些事情. */
	if (sock->sk) sock->sk->sock = NULL;
	kfree(sock);
	sock == NULL;
	release_lock(&slock);
	return 0;
}

static struct socket *
get_socket(pid_t pid, int fd)
{
	struct list_head *item;
	struct socket *sock = NULL;
	acquire_lock(&slock);
	list_for_each(item, &sockets) {
		sock = list_entry(item, struct socket, list);
		if (sock->pid == pid && sock->fd == fd) {
			release_lock(&slock);
			return sock;
		}
	}
	release_lock(&slock);
	return NULL;
}

/**\ 
 * 以下的一系列函数和我们经常使用的函数非常类似,确实,这里确实是在试图还原一系列的网络系统调用.
 * 下面的函数一般都是调用sock->ops->xxx,ops指的是操纵socket的一系列函数.这里的xxx,
 * 函数接口和我们经常使用的基本上是一致的. 
 *
 * 这里可以保证,传入的参数全部是有效的.所以可以删除掉错误检查的代码.
\**/


/**\
 * _socket函数构建一个socket,并且将其加入到connections这个链表之中.
\**/

int _socket(pid_t pid, int domain, int type, int protocol) {
	struct socket *sock;
	struct net_family *family;
	if ((sock = alloc_socket(pid)) == NULL) {
		kprintf("Could not alloc socket\n");
		return -1;
	}
	sock->type = type;
	family = families[domain];

	if (!family) {
		kprintf("Domain not supprted: %u\n", domain);
		goto abort_socket;
	}

	if (family->create(sock, protocol) != 0) {	/* 构建一个sock,这里的create
										在这个栈中,实际调用inet_create函数 */
		kprintf("Creating domain failed\n");
		goto abort_socket;
	}

	socket_sockets_enqueue(sock); /* 将新构建的socket放入connections链中 */
	return sock->fd;  /* sock->fd只是一个标志而已 */

abort_socket:
	free_socket(sock);
	return -1;
}

int _bind(pid_t pid, int sockfd, struct sockaddr_in *skaddr) {
	struct socket *sock;

	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Bind: could not find socket (fd %u) for binding (pid %u)\n",
			sockfd, pid);
		return -1;
	}
	return sock->ops->bind(sock, skaddr);	/* 实际上调用了inet_bind */
}

int _connect(pid_t pid, int sockfd, const struct sockaddr_in *addr) {
	struct socket *sock;

	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Connect: could not find socket (fd %u) for connection (pid %u)\n",
		sockfd, pid);
		return -1;
	}
	return sock->ops->connect(sock, addr);
}

/**\
 * write 向pid进程连接的第sockfd号文件写入数据.
\**/
int _write(pid_t pid, int sockfd, const void *buf, const uint32_t count) {
	struct socket *sock;
	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Write: could not find socket (fd %u && pid %u)\n",
			sockfd, pid);
		return -1;
	}
	return sock->ops->write(sock, buf, count);
}

int _read(pid_t pid, int sockfd, void* buf, const uint32_t count) {
	struct socket *sock;
	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Read: could not find socket (fd %u && pid %u)\n",
			sockfd, pid);
		return -1;
	}
	return sock->ops->read(sock, buf, count);
}

int _sendto(pid_t pid, int sockfd, const void *buf, int len, 
    const struct sockaddr_in *saddr) {
	struct socket *sock;
	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Sendto: could not find socket (fd %u && pid %u)\n",
			sockfd, pid);
		return -1;
	}
	// tofix
	return sock->ops->sendto(sock, buf, len, saddr);
}

int _close(pid_t pid, int sockfd) {
	struct socket *sock;
	int rc = -1;
	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Close: could not find socket (fd %u && pid %u)\n",
			sockfd, pid);
		return -1;
	}
	rc = sock->ops->close(sock);
	if (rc == 0) {
		socket_sockets_remove(sock); /* 将其从socks链表中删除 */
		free_socket(sock);
	}
	return rc;
}

int _listen(pid_t pid, int sockfd, int backlog) {
	int err = -1;
	struct socket *sock;
	if (backlog < 0) goto out;
	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("listen: could not find socket (fd %u) for \
			listening (pid %u)\n",
			sockfd, pid);
		return -1;
	}
	return sock->ops->listen(sock, backlog);
out:
	return err;
}


int _accept(pid_t pid, int sockfd, struct sockaddr_in *skaddr) {
	struct socket * sock, *newsock;
	int rc = -1, err;

	if ((sock = get_socket(pid, sockfd)) == NULL) {
		kprintf("Accept: could not find socket (fd %u && pid %u)\n",
			sockfd, pid);
		return -1;
	}
	newsock = alloc_socket(pid);
	newsock->ops = sock->ops;

	err = sock->ops->accept(sock, newsock, skaddr);
	if (err < 0) {
		kfree(newsock);
		newsock = NULL;
	}
	else {
		socket_sockets_enqueue(newsock);
		rc = newsock->fd;	/* 返回对应的文件描述符 */
	}
out:
	return rc;
}
