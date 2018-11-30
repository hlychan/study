#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "https_client.h"

char *url = "";

char *http_hdr = "POST %s HTTP/1.1\r\n"
		 "User-Agent: Mozilla/4.0\r\n"
		 "Host: %s:%d\r\n"
		 "Accept: */*\r\n"
		 "Content-Type: application/x-www-form-urlencoded\r\n"
		 "Content-Length: %d\r\n\r\n";

int tcp_connect(struct sockaddr_in server)
{
	int skfd;
	struct timeval timeout;

	if ((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return skfd;

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	setsockopt(skfd, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(struct timeval));

	if (connect(skfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0)
		return -1;

	return skfd;
}

int ssl_connect(ssl_ctx *context)
{
	// Register the error strings for libcrypto & libssl
	SSL_load_error_strings ();

	// Register the available ciphers and digests
	SSL_library_init ();
	OpenSSL_add_all_algorithms();

	// New context saying we are a client, and using SSL 2 or 3
	//context->ctx = SSL_CTX_new (SSLv23_client_method());
	context->ctx = SSL_CTX_new (TLSv1_2_client_method());
	if (context->ctx == NULL) {
		printf("SSL_CTX_new error\n");
		return -1;
	}

	// Create an SSL struct for the connection
	context->ssl = SSL_new (context->ctx);
	if (context->ssl == NULL) {
		printf("SSL_new error\n");
		return -1;
	}

	// Connect the SSL struct to our connection
	if (!SSL_set_fd (context->ssl, context->skfd)) {
		printf("SSL_set_fd error\n");
		return -1;
	}

	// Initiate SSL handshake
	if (SSL_connect (context->ssl) != 1) {
		printf("SSL_connect error\n");
		return -1;
	}

	return 0;
}

void ssl_disconnect(ssl_ctx context)
{
	if (context.skfd)
		close(context.skfd);
	if (context.ssl) {
		SSL_shutdown(context.ssl);
		SSL_free(context.ssl);
	}
	if (context.ctx)
		SSL_CTX_free(context.ctx);
}

int ssl_write(ssl_ctx *context, const char *data, int len)
{
	if (context->ssl)
		return SSL_write(context->ssl, data, len);
	return -1;
}

char *ssl_read(ssl_ctx *context)
{
	int read_size = 2048;
	char *rc;
	int received = 0, count = 0;
	char buffer[2048];

	if (context) {
		while (1) {
			if (!rc) {
				rc = malloc (read_size * sizeof (char) + 1);
				if (rc == NULL)
					goto err;
			} else {
				rc = realloc (rc, (count + 1) * read_size * sizeof (char) + 1);
				if (rc == NULL)
					goto err;
			}
			memset(rc+count*read_size, 0, read_size+1);
			memset(buffer, 0, read_size);
			received = SSL_read(context->ssl, buffer, read_size);
			buffer[received] = '\0';

			if(received < 0)
				goto err;
			if (received > 0)
				strcat (rc, buffer);
			if (received < read_size)
				return rc;

			count++;
		}
	}

err:
	if (rc) {
		free(rc);
		rc = NULL;
	}

	return NULL;
}

void show_usage(const char *program)
{
	printf("Usage: %s [argument]\n", program);
	printf("    Where arguments is optional as:\n");
	printf("\t-a server address\n");
	printf("\t-f upload file name\n");
	printf("\t-p server port\n");
	printf("\t-h show usage\n");
}

int main(int argc, char **argv)
{
	int arg;
	int port = 443;
	char filename[64] = {0};
	struct stat stats;
	int filesize = 0;
	int len = 0;
	char sendbuf[1024] = {0};
	char ip[64];
	FILE *fp;
	ssl_ctx context;

	memset(&context, 0, sizeof(context));
	context.ssl = NULL;
	context.ctx = NULL;
	context.addr.sin_family = AF_INET;

	if (argc <= 1) {
		show_usage(argv[0]);
		return -1;
	}

	while (1) {
		arg = getopt(argc, argv, "a:f:p:h");

		if (arg < 0)
			break;

		switch (arg) {
		case 'a':
			snprintf(ip, sizeof(ip), "%s", optarg);
			break;
		case 'f':
			snprintf(filename, sizeof(filename), "%s", optarg);
			break;
		case 'p':
			sscanf(optarg, "%d", &port);
			context.addr.sin_port = htons(port);
			break;
		case 'h':
			show_usage(argv[0]);
			break;
		default:
			show_usage(argv[0]);
			return -1;
		}
	}

	context.addr.sin_addr.s_addr = inet_addr(ip);
	context.skfd = tcp_connect(context.addr);
	if (context.skfd <= 0) {
		perror("tcp_connect");
		exit(1);
	}
	if (ssl_connect(&context) != 0) {
		printf("ssl_connect error\n");
		return -1;
	}

	stat(filename, &stats);
	filesize = stats.st_size;

	// send http header
	memset(sendbuf, 0, sizeof(sendbuf));
	snprintf(sendbuf, sizeof(sendbuf), http_hdr, url, ip, port, filesize);
	printf("send to server:\n%s", sendbuf);
	len = strlen(sendbuf);
	printf("header len: %d\n", len);
	ssl_write(&context, sendbuf, strlen(sendbuf));

	// send content to server
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("open file error\n");
		return -1;
	}
	memset(sendbuf, 0, sizeof(sendbuf));
	while (fgets(sendbuf, sizeof(sendbuf), fp)) {
		printf("%s", sendbuf);
		len = strlen(sendbuf);
		ssl_write(&context, sendbuf, strlen(sendbuf));
		memset(sendbuf, 0, sizeof(sendbuf));
	}
	
	char *recv = ssl_read(&context);
	if (recv != NULL) {
		printf("receive from server:\n%s\n", recv);
	}

	//ssl_disconnect(context);

	return 0;
}

