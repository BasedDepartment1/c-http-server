#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

const int MAX_CONNS = 1024;
#define RECV_BUF_SIZE 4096

int main() {
	int code = EXIT_SUCCESS;
	setbuf(stdout, NULL);
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket creation failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in addrport;
	memset(&addrport, 0, sizeof(addrport));

	addrport.sin_family = AF_INET;
	addrport.sin_port = htons(5000);
	addrport.sin_addr.s_addr = htons(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
		fprintf(stderr, "Addr bind failed: %s\n", strerror(errno));
		code = EXIT_FAILURE;
		goto cleanup;
	}

	int status = listen(sockfd, MAX_CONNS);
	if (status < 0) {
		fprintf(stderr, "Server start failed: %s\n", strerror(errno));
		code = EXIT_FAILURE;
		goto cleanup;
	}

	for (;;) {
		int clientfd;
		struct sockaddr client_addr;
		int addr_len = sizeof(client_addr);
		
		clientfd = accept(sockfd, &client_addr, &addr_len);
		if (clientfd < 0) {
			fprintf(stderr, "accept failed: %s\n", strerror(errno));
			code = EXIT_FAILURE;
			goto cleanup;
		}

		struct sockaddr_in *c_addr_in = (struct sockaddr_in *) &client_addr;
		char c_ip_addr[INET_ADDRSTRLEN] = {0};
		inet_ntop(c_addr_in->sin_family, &(c_addr_in->sin_addr), c_ip_addr, INET_ADDRSTRLEN);
		int c_port = ntohs(c_addr_in->sin_port);

		printf("%s:%d opened connection\n", c_ip_addr, c_port);

		char buf[RECV_BUF_SIZE] = {0};
		int recv_msg_size = recv(clientfd, &buf, RECV_BUF_SIZE-1, 0);
		if (recv_msg_size < 0) {
			fprintf(stderr, "recv failed: %s\n", strerror(errno));
			code = EXIT_FAILURE;
			goto clean_client;
		}

		for (;recv_msg_size > 0;) {
			for (int sent = 0; sent != recv_msg_size;) {
				int cur_sent = send(clientfd, &buf, recv_msg_size - sent, 0);
				if (cur_sent < 0) {
					fprintf(stderr, "send failed: %s\n", strerror(errno));
					code = EXIT_FAILURE;
					goto clean_client;
				}
				sent += cur_sent;
			}

			printf("%s", buf);
			memset(&buf, 0, RECV_BUF_SIZE);
			recv_msg_size = recv(clientfd, &buf, RECV_BUF_SIZE-1, 0);
			if (recv_msg_size < 0) {
				fprintf(stderr, "recv failed: %s\n", strerror(errno));
				code = EXIT_FAILURE;
				goto clean_client;
			}
		}

	clean_client:
		close(clientfd);
		printf("\n%s:%d closed connection\n\n", c_ip_addr, c_port);
		if (code == EXIT_SUCCESS) {
			continue;
		}
		goto cleanup;
	}

cleanup:
	close(sockfd);
	return code;
}
