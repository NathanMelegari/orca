#include "orca_http.h"
#include <linux/in.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define IP_SERVER_DEFAULT "::"

typedef struct Server {
	int fd;
	uint32_t port;
} Server;

static int init(uint32_t port);
static bool sendAll(int fd, const void *buffer, size_t length);
static bool recAll(int fd, void *buffer, size_t capacity, size_t *received);
static ssize_t recSome(int fd, void *buffer, size_t capacity);

bool orcaListenAndServe(OrcaApp *app, uint32_t port) {
	if(app == NULL) {
		fprintf(stderr, "Error: context is NULL\n");
		return false;
	}

	Server server = {
		.fd = -1,
		.port = port
	};

	server.fd = init(server.port);
	if(server.fd < 0) {
		fprintf(stderr, "Error: failed to init server\n");
		return false;
	}

	printf("Server running on %u\n", server.port);

	while(app->is_running) {
		struct sockaddr_in6 clientAddr = {0};
		socklen_t length = sizeof(clientAddr);

		int clientFd = accept(
				server.fd,
				(struct sockaddr*)&clientAddr,
				&length
		);
		if(clientFd < 0) {
			if(errno == EINTR) continue;
			perror("accept");
			break;
		}

		int flag = 1;
		setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

		char requestBuffer[8192];

		ssize_t received = recSome(clientFd, requestBuffer, sizeof(requestBuffer));
		if(received <= 0) {
			close(clientFd);
			continue;
		}

		OrcaRequest request = {0};
		OrcaResponse response = {0};

		if(!orcaRequestParse(requestBuffer, (size_t)received, &request)) {
			close(clientFd);
			continue;
		}

		OrcaHandler handler = orcaRouterResolver(&app->router, &request);
		if(handler == NULL) {
			response.status = 404;
			response.contentType = "application/json";
			response.body = "{\"error\": \"Not Found\"}";
			response.bodyLength = strlen(response.body);
		} else {
			handler(&request, &response);
		}

		char *responseBuffer = NULL;
		size_t responseLength = 0;
		
		if(!orcaResponseSerialize(&response, &responseBuffer, &responseLength)) {
			response = (OrcaResponse){
				.status = 500,
				.contentType = "application/json",
				.body = "{\"error\": \"Internal Server Error\"}",
				.bodyLength = strlen("{\"error\": \"Internal Server Error\"}")
			};
			if(!orcaResponseSerialize(&response, &responseBuffer, &responseLength)) {
				close(clientFd);
				orcaRequestFree(&request);
				continue;
			}
		}

		if(responseBuffer != NULL) {
			if(!sendAll(clientFd, responseBuffer, responseLength)) {
				perror("sendAll");
			}
			free(responseBuffer);
		}

		orcaRequestFree(&request);
		close(clientFd);
		clientFd = -1;
	}

	close(server.fd);
	server.fd = -1;
	return true;
}

static int init(uint32_t port) {

#ifdef __linux__
	int fd = socket(AF_INET6, SOCK_STREAM, 0);
	if(fd < 0) {
		perror("socket");
		return -1;
	}

	int opt = 1;

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(fd);
		fd = -1;
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in6 addr = {0};
	addr.sin6_port = htons(port);
	addr.sin6_family = AF_INET6;
	if(inet_pton(AF_INET6, IP_SERVER_DEFAULT, &addr.sin6_addr) != 1) {
		fprintf(stderr, "Error: inet_pton has failed\n");
		close(fd);
		fd = -1;
		return -1;
	}

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(fd);
		fd = -1;
		fprintf(stderr, "Error: failed to bind fd\n");
		return -1;
	}

	if(listen(fd, 10) < 0) {
		close(fd);
		fd = -1;
		fprintf(stderr, "Error: failed to listen\n");
		return -1;
	}
#else
	return -1;
#endif

	return fd;
}

static bool sendAll(int fd, const void *buffer, size_t length) {
	if(fd < 0 || buffer == NULL) return false;

	const char *data = buffer;
	size_t total = 0;

	while(total < length) {

		ssize_t sent = send(fd, data + total, length - total, MSG_NOSIGNAL);
		if(sent < 0) {
			if(errno == EINTR) continue;
			perror("send");
			return false;
		}
		if(sent == 0) {
			perror("send");
			return false;
		}

		total += (size_t)sent;
	}

	return true;
}

static ssize_t recSome(int fd, void *buffer, size_t capacity) {
	if(fd < 0 || buffer == NULL || capacity == 0) return -1;

	ssize_t received;

	do {
		received = recv(fd, buffer, capacity, 0);

	} while(received < 0 && errno == EINTR);

	return received;
}
