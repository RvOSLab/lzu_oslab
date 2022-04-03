#include <net/net_utils.h>
#include <net/tcp.h>
#include <net/ip.h>
#include <net/skbuff.h>
#include <net/timer.h>
#include <sched.h>
#include <errno.h>

static void tcp_retransmission_timeout(uint32_t ts, void *arg);

static struct sk_buff *tcp_alloc_skb(int optlen, int size) {

	/* optlen表示tcp首部选项的大小 */
	int reserved = ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN + optlen + size; 
	struct sk_buff *skb = alloc_skb(reserved);

	skb_reserve(skb, reserved); /* skb->data部分留出reserved个字节 */
	skb->protocol = IP_TCP;
	skb->dlen = size;	/* dlen表示数据的大小 */

	return skb;
}

static int tcp_write_options(struct tcphdr *th, struct tcp_options *opts, int optlen) {
	struct tcp_opt_mss *opt_mss = (struct tcp_opt_mss *)th->data;

	opt_mss->kind = TCP_OPT_MSS;
	opt_mss->len = TCP_OPTLEN_MSS;
	opt_mss->mss = htons(opts->mss);

	th->hl = TCP_DOFFSET + (optlen / 4);
	return 0;
}

static int tcp_syn_options(struct sock *sk, struct tcp_options *opts) {
	struct tcp_sock *tsk = tcp_sk(sk);
	int optlen = 0;

	opts->mss = tsk->rmss;
	optlen += TCP_OPTLEN_MSS;
	return optlen;
}

static int tcp_transmit_skb(struct sock *sk, struct sk_buff *skb, uint32_t seq) {
	struct tcp_sock *tsk = tcp_sk(sk);
	struct tcb *tcb = &tsk->tcb;
	struct tcphdr *thdr = tcp_hdr(skb);  /* tcp头部信息 */

	if (thdr->hl == 0) thdr->hl = TCP_DOFFSET;

	skb_push(skb, thdr->hl * 4); /* hl表示tcp头部大小 */

	/* 填充头部信息 */
	thdr->sport = sk->sport;
	thdr->dport = sk->dport;
	thdr->seq = seq;		/* 分组的序号 */
	thdr->ack_seq = tcb->rcv_nxt; /* 携带一个确认序号 */
	// thdr->win = tcb->rcv_wnd;	  /* 接收窗口的大小 */
	thdr->win = 444777;
	thdr->csum = 0;
	thdr->urp = 0;
	thdr->rsvd = 0;

	/* 记录下要传递的数据包的起始和终止序号,这里主要是为了方便后面的重传
	 具体可以参见tcp_input.c --> tcp_clean_retransmission_queue 函数. */
	skb->seq = tcb->snd_una;
	skb->end_seq = tcb->snd_una + skb->dlen; /* 终止序列号 */

	thdr->sport = htons(thdr->sport);
	thdr->dport = htons(thdr->dport);
	thdr->seq = htonl(thdr->seq);
	thdr->ack_seq = htonl(thdr->ack_seq);
	thdr->win = htons(thdr->win);
	thdr->csum = htons(thdr->csum);
	thdr->urp = htons(thdr->urp);
	// tcp首部的检验和,需要检验首部和数据,在计算检验和时,要在TCP报文段的前面加上12字节伪首部
	/*
		TCP伪首部
	          4                     4              1     1       2
	   +--------------------+-------------------+-----+-----+----------+
	   | saddr              | daddr             | 0   | 6   | len      |
	   +--------------------+-------------------+-----+-----+----------+
	   
	 */
	thdr->csum = tcp_checksum(skb, htonl(sk->saddr), htonl(sk->daddr));

	return ip_output(sk, skb);
}

/**\
 * tcp_queue_transmit_skb 将要发送的内容加入write_queue,没有接收到ack的话,会一直重
 * 传该数据包,而tcp_transmit_skb函数不会如此. 
\**/
static int tcp_queue_transmit_skb(struct sock *sk, struct sk_buff *skb) {
	struct tcp_sock *tsk = tcp_sk(sk);
	struct tcb *tcb = &tsk->tcb;  /* 传输控制块 */
	int rc = 0;

	acquire_lock(&sk->write_queue.lock);

	if (skb_queue_empty(&sk->write_queue)) {
		tcp_reset_retransmission_timer(tsk);
	}

	skb_queue_tail(&sk->write_queue, skb);	/* 将skb加入到发送队列的尾部 */
	rc = tcp_transmit_skb(sk, skb, tcb->snd_nxt); /* 首先将数据发送一遍 */
	tcb->snd_nxt += skb->dlen;
	release_lock(&sk->write_queue.lock);
	return rc;
}

int tcp_send_synack(struct sock *sk) {
	if (sk->state != TCP_SYN_RECEIVED) {
		kprintf("TCP synack: Socket was not in correct state (SYN_SENT)\n");
		return 1;
	}

	struct sk_buff *skb;
	struct tcphdr *th;
	struct tcb *tcb = &tcp_sk(sk)->tcb;
	int rc = 0;

	skb = tcp_alloc_skb(0, 0);
	th = tcp_hdr(skb);

	th->syn = 1;
	th->ack = 1;
	/* 不消耗序列号 */
	rc = tcp_transmit_skb(sk, skb, tcb->snd_nxt);
	free_skb(skb);

	return rc;
}

void tcp_send_delack(uint32_t ts, void *arg) {
	struct sock *sk = (struct sock *)arg;
	struct tcp_sock *tsk = tcp_sk(sk);

	tsk->delacks = 0;
	tcp_release_delack_timer(tsk);
	tcp_send_ack(sk);
}

/**\
 * tcp_send_ack 发送ack.
\**/
int tcp_send_ack(struct sock *sk) {
	if (sk->state == TCP_CLOSE) 
		return 0;

	struct sk_buff *skb;
	struct tcphdr *th;
	struct tcb *tcb = &tcp_sk(sk)->tcb;
	int rc = 0;

	skb = tcp_alloc_skb(0, 0);  /* 需要重新分配数据块,不包含tcp选项,不包含数据 */

	th = tcp_hdr(skb); /* th指向要发送数据的tcp头部 */
	th->ack = 1;

	rc = tcp_transmit_skb(sk, skb, tcb->snd_nxt);
	free_skb(skb);

	return rc;
}

static int tcp_send_syn(struct sock *sk) {
	if (sk->state != TCP_SYN_SENT && sk->state != TCP_CLOSE && sk->state != TCP_LISTEN) {
		kprintf("Socket was not in correct state (closed or listen)\n");
		return 1;
	}

	struct sk_buff *skb;
	struct tcphdr *th;
	struct tcp_options opts = { 0 };
	int tcp_options_len = 0;

	tcp_options_len = tcp_syn_options(sk, &opts);	/* tcp选项的长度 */
	skb = tcp_alloc_skb(tcp_options_len, 0); /* 需要发送tcp选项 */
	th = tcp_hdr(skb);		/* 指向tcp头部 */

	tcp_write_options(th, &opts, tcp_options_len);
	sk->state = TCP_SYN_SENT;  /* 客户端发送了syn之后,进入syn_sent状态 */
	th->syn = 1;

	return tcp_queue_transmit_skb(sk, skb);
}

int tcp_send_fin(struct sock *sk) {
	if (sk->state == TCP_CLOSE) return 0;
	struct tcphdr *th;
	struct sk_buff *skb;
	int rc = 0;

	skb = tcp_alloc_skb(0, 0);

	th = tcp_hdr(skb);
	th->fin = 1;
	th->ack = 1;

	rc = tcp_queue_transmit_skb(sk, skb);

	return rc;
}

void tcp_select_initial_window(uint32_t *rcv_wnd) {
	*rcv_wnd = 44477;
}

static void tcp_notify_user(struct sock *sk) {
	struct tcp_sock *tsk = tcp_sk(sk);
	switch (sk->state) {
	case TCP_CLOSE_WAIT:
		wake_up(&tsk->wait);
		break;
	}
}

static void tcp_connect_rto(uint32_t ts, void *arg) {
	struct tcp_sock *tsk = (struct tcp_sock *)arg;
	struct tcb *tcb = &tsk->tcb;
	struct sock *sk = &tsk->sk;

	tcp_release_retransmission_timer(tsk);

	if (sk->state != TCP_ESTABLISHED) {
		if (tsk->backoff > TCP_CONN_RETRIES) { /* 如果退避的次数过多,表示连接不通,
											   要通知上层. */
			tsk->sk.err = -ETIMEDOUT;  /* 超时 */
			// tofix: 要将sk从链表中移除
			tcp_free_sock(sk);
		}
		else {
			acquire_lock(&sk->write_queue.lock);

			struct sk_buff *skb = write_queue_head(sk);

			if (skb) {
				skb_reset_header(skb);
				tcp_transmit_skb(sk, skb, tcb->snd_una);

				tsk->backoff++;
				tcp_reset_retransmission_timer(tsk);
			}
			release_lock(&sk->write_queue.lock);
		}
	}
	else {
		kprintf("TCP connect RTO triggered even when Established\n");
	}

}

/**\
 * tcp_retransmission_timeout 如果在规定的时间内还没有收到tcp数据报的确认,那么要重传
 * 该数据包. 
\**/
static void tcp_retransmission_timeout(uint32_t ts, void *arg) {
	struct tcp_sock *tsk = (struct tcp_sock *)arg;
	struct tcb *tcb = &tsk->tcb;
	struct sock *sk = &tsk->sk;

	acquire_lock(&sk->write_queue.lock);
	tcp_release_retransmission_timer(tsk);

	struct sk_buff *skb = write_queue_head(sk);

	if (!skb) {
		tcp_notify_user(sk);
		goto unlock;
	}

	struct tcphdr *th = tcp_hdr(skb);
	skb_reset_header(skb);

	tcp_transmit_skb(sk, skb, tcb->snd_una);
	/* 每个500个时间单位检查一次. */
	tsk->retransmit = timer_add(500, &tcp_retransmission_timeout, tsk);

	if (th->fin) {
		tcp_handle_fin_state(sk);
	}

unlock:
	release_lock(&sk->write_queue.lock);
}

/**\
 * tcp_reset_retransmission_timer 用于重新设置重传定时器.
\**/
void tcp_reset_retransmission_timer(struct tcp_sock *tsk) {
	struct sock *sk = &tsk->sk;
	tcp_release_retransmission_timer(tsk);	/* 释放掉之前的重传定时器 */

	if (sk->state == TCP_SYN_SENT) {	/* backoff 貌似是退避时间 */
		tsk->retransmit = timer_add(TCP_SYN_BACKOFF << tsk->backoff, &tcp_connect_rto, tsk);
	}
	else {
		/* 500秒超时重传 */
		tsk->retransmit = timer_add(500, &tcp_retransmission_timeout, tsk);
	}
}

int tcp_begin_connect(struct sock *sk) {
	struct tcp_sock *tsk = tcp_sk(sk);
	struct tcb *tcb = &tsk->tcb;
	int rc = 0;

	tsk->tcp_header_len = sizeof(struct tcphdr);
	tcb->isn = tcp_generate_isn();  /* isn是随机产生的一个序列号 */
	tcb->snd_wnd = 0;
	tcb->snd_wl1 = 0;

	tcb->snd_una = tcb->isn;
	tcb->snd_up = tcb->isn;	
	tcb->snd_nxt = tcb->isn;
	tcb->rcv_nxt = 0;

	tcp_select_initial_window(&tsk->tcb.rcv_wnd); /* 接收窗口的大小 */

	rc = tcp_send_syn(sk);  /* tcp_send_syn可能由于暂时找不到以太网地址的原因发送失败
							但是存在定时器,隔一段时间再次尝试发送. */
	tcb->snd_nxt++;  /* 消耗一个序列号 */
	return rc;
}

/* tcp_send 发送tcp数据 */
int  tcp_send(struct tcp_sock *tsk, const void *buf, int len)
{
	struct sk_buff *skb;
	struct tcphdr *th;
	int slen = len;
	int mss = tsk->smss;
	int dlen = 0;

	while (slen > 0) {
		dlen = slen > mss ? mss : slen; /* 一个tcp报文最多只能发送mss个字节tcp数据 */
		slen -= dlen;

		skb = tcp_alloc_skb(0, dlen); /* tcp头部选项0字节,数据大小dlen字节 */
		skb_push(skb, dlen);
		memcpy(skb->data, buf, dlen);

		buf += dlen;
		th = tcp_hdr(skb);
		th->ack = 1;

		if (slen == 0) {
			th->psh = 1;	/*将推送标志bit置为1,表示接收方一旦接收到这个报文,
							就应该尽快将数据推送给应用程序 */
		}

		if (tcp_queue_transmit_skb(&tsk->sk, skb) == -1) {
			kprintf("Error on TCP skb queueing");
		}
	}
	return len;
}

/* tcp_send_reset 向对端发送rst */
int tcp_send_reset(struct tcp_sock *tsk) {
	struct sk_buff *skb;
	struct tcphdr *th;
	struct tcb *tcb;
	int rc = 0;

	skb = tcp_alloc_skb(0, 0);
	th = tcp_hdr(skb);
	tcb = &tsk->tcb;

	th->rst = 1;
	tcb->snd_una = tcb->snd_nxt;

	/* rst 并不消耗序列号 */
	rc = tcp_transmit_skb(&tsk->sk, skb, tcb->snd_nxt);
	free_skb(skb);
	return rc;
}

int tcp_queue_fin(struct sock *sk) {
	struct sk_buff *skb;
	struct tcphdr *th;
	struct tcb *tcb = &tcp_sk(sk)->tcb;
	int rc = 0;

	skb = tcp_alloc_skb(0, 0);  /* 需要重新分配数据块,不包含tcp选项,不包含数据 */
	th = tcp_hdr(skb);

	th->fin = 1;
	th->ack = 1;

	/* 调用tcp_queue_transmit_skb,如果没有及时收到对方对该数据报的ack,会重传该数据包 */
	rc = tcp_queue_transmit_skb(sk, skb);
	/* TCP规定,fin报文即使不携带数据,它也要消耗掉一个序号 */
	tcb->snd_nxt++;	/* fin消耗一个序列号 */

	return rc;
}
