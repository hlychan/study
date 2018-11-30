#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

typedef struct ssl_cxt_s {
	int skfd;
	struct sockaddr_in addr;
	SSL *ssl;
	SSL_CTX *ctx;
} ssl_ctx;

int tcp_connect(struct sockaddr_in);
int ssl_connect(ssl_ctx *);
void ssl_disconnect(ssl_ctx);
int ssl_write(ssl_ctx *, const char *, int);
char *ssl_read(ssl_ctx *);
void show_usage(const char *);

#endif

