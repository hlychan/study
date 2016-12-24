/*
 * Send http request to web server, record website loaded time and 
 * packets retransmitted.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#define HOST "www.baidu.com"
#define PAGE "/"
#define PORT 80
#define USERAGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.114 Safari/537.36"
#define ACCEPTLANGUAGE "zh-CN,zh;q=0.8,en;q=0.6,en-US;q=0.4,en-GB;q=0.2"
#define ACCEPTENCODING "gzip,deflate,sdch"

static int create_tcp_socket(void);
static char **resolvename(char *);
static char *build_get_query(char *, char *);

int main(int argc, char **argv)
{
	struct sockaddr_in *server;
	int sock;
	char **serverIP = NULL;
	int result = -1;
	char *get, *page;
	char buf[65536];
    
    sock = create_tcp_socket();  
    serverIP = resolvename(argv[1]); 
	
	// set server msg
	server = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    server->sin_family = AF_INET;
	server->sin_port = htons(80);
	
	while (*serverIP != NULL) {
		if ((result = inet_pton(AF_INET, *serverIP, (void *)(&(server->sin_addr.s_addr)))) <= 0) {
			printf("Set server IP failed!\n");
			exit(1);
		}
	    if (connect(sock, (struct sockaddr *)server, sizeof(struct sockaddr)) < 0) {
	        perror("Connect");
			*serverIP++;
	    } else {
			printf("Connect to server success, Address:%s\n", (char *)inet_ntoa(server->sin_addr));
			break;
		}
	}
 
    page = PAGE;
	get = build_get_query(argv[1], page);
	fprintf(stdout, "<start>\n%s\n<end>\n", get);

	int sent = 0;
	while (sent < strlen(get)) {
	    result = send(sock, (get + sent), (strlen(get) - sent), 0);
		if (result == -1) {
		    perror("send:");
			exit(1);
		}
		sent += result;
	}

	memset(buf, 0, sizeof(buf));
	int htmlstart = 0;
	char *htmlcontent;
	while ((result = recv(sock, buf, 65535, 0)) > 0) {
	    if (htmlstart == 0) {
		    htmlcontent = strstr(buf, "\r\n\r\n");
			if (htmlcontent != NULL) {
			    htmlstart = 1;
				htmlcontent += 4;
			}
		} else {
		    htmlcontent = buf;
		}

		if (htmlstart) {
		    fprintf(stdout, "%s", htmlcontent);
		}
		memset(buf, 0, result);
	}
	fprintf(stdout, "Receive data over!\n");
	
	if (result < 0) {
	    perror("Error receiving data");
	}

	free(get);
	free(server);
	free(serverIP);
	close(sock);
	 
	return 0;
}

static int create_tcp_socket(void)
{
	int s;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    perror("Socket:");
		exit(1);
	}
	
	return s;
}

static char **resolvename(char *domain_name)
{
	struct hostent *host;
	char **res_addr;
	int ipLen = 15;
	int i, count = 0;
    
	if ((host = gethostbyname(domain_name)) == NULL) {
		printf("Resolve domain name failed!\n");
		exit(1);
	}
	
	for (i = 0; host->h_addr_list[i] != 0; i++) count++;
	
	res_addr = (char **)malloc(sizeof(char *) * count);
	for (i = 0; host->h_addr_list[i] != 0; i++) {
		res_addr[i] = (char *)malloc(ipLen + 1);
	    if (inet_ntop(AF_INET, (void *)host->h_addr_list[i], res_addr[i], ipLen) == 0) {
		    printf("Trans IP to ASCII failed!\n");
		    exit(1);
	    }
	}
	for (i = 0; res_addr[i] != NULL; i++) {
	    printf("Resolve domain name success!\nAddress:%s\n", res_addr[i]);
	}
	
	return res_addr;
}

static char *build_get_query(char *host, char *page)
{
	char *query;
	//char *agent = USERAGENT;
	//char *lang = ACCEPTLANGUAGE;
	char *getpage = page;
	char *tp1 = "GET %s HTTP/1.1\r\nHost:%s\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent:%s\r\nAccept-Language:%s\r\n\r\n";
    int size = strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tp1)+strlen(ACCEPTLANGUAGE)-5;
	query=(char *)malloc(size);
    sprintf(query, tp1, getpage, host, USERAGENT, ACCEPTLANGUAGE);

	return query;
}
