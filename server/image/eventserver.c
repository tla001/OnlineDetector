/*
 * eventserver.c
 *
 *  Created on: Jun 13, 2017
 *      Author: tla001
 */
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//iso
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//others
#include <event2/event-config.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_struct.h>
#include <event2/buffer_compat.h>

#include "cJSON.h"

void test_request_cb(struct evhttp_request *req, void *arg) {
	int ppid=getppid();
	int type = evhttp_request_get_command(req);
	const char *requestUri = evhttp_request_get_uri(req);
	if (EVHTTP_REQ_GET == type) {
		printf("method:GET uri:%s\n", requestUri);
	} else if (EVHTTP_REQ_POST == type) {
		printf("method:POST uri:%s\n", requestUri);
	}

	char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
//	printf("post data: %s", post_data);
	size_t maxSize = 0;
	memcpy(&maxSize, post_data, sizeof(size_t));


	FILE *fp = fopen("test.jpg", "wb");
	fwrite(post_data + sizeof(size_t), 1, maxSize, fp);
	fclose(fp);
	kill(ppid,SIGUSR1);
	const char *FIFO_NAME="/tmp/myfifo";
	int fifo_fd=open(FIFO_NAME,O_RDONLY);
	char tmp=0;
	int res=read(fifo_fd,&tmp,1);
	if(res==-1){
		printf("read err\n");
		goto THISEXIT;
	}
	close(fifo_fd);
	printf("fifo tmp=%c\n", tmp);
	char *resData="rec";
	if(tmp=='1'){
		FILE *fp=fopen("res.json","rb");
		if(fp==NULL)
			goto THISEXIT;
		fseek(fp,0,SEEK_END);
		size_t size=ftell(fp);
		rewind(fp);
		resData=NULL;
		resData=(char*)malloc(sizeof(char)*size+1);
		int readSize=fread(resData,1,size,fp);
		if(readSize!=size){
			printf("read err\n");
		}
		resData[sizeof(char)*size]='\0';
		printf("%s\n", resData);
		fclose(fp);
	}
	


	printf("rec data len:%d\n", strlen(resData));
	struct evbuffer *buf1 = evbuffer_new();
	evbuffer_add_printf(buf1, resData);
	evhttp_send_reply(req, 200, "OK", buf1);
	if(resData&&tmp=='1')
		free(resData);
	return ;
THISEXIT:
	kill(ppid,SIGINT);
		
	exit(-1);
}
void ServerRun() {
	int port = 5555;

	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		printf("signal error,error[%d],error[%s]", errno, strerror(errno));
		exit(-1);
	}
	base = event_base_new();
	if (!base) {
		printf("create an event_base err\n");
		exit(-1);
	}
	http = evhttp_new(base);
	if (!http) {
		printf("create evhttp err\n");
		exit(-1);
	}
	evhttp_set_cb(http, "/test", test_request_cb, NULL);

	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		printf("bind to port[%d] err\n", port);
		exit(-1);
	}

	{
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;
		fd = evhttp_bound_socket_get_fd(handle);
		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr*) &ss, &socklen)) {
			perror("getsockname failed");
			exit(-1);
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*) &ss)->sin_port);
			inaddr = &((struct sockaddr_in*) &ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*) &ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*) &ss)->sin6_addr;
		} else {
			printf("Weird address family\n");
			exit(1);
		}

		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf, sizeof(addrbuf));
		if (addr) {
			printf("Listening on %s:%d\n", addr, got_port);
		} else {
			printf("evutil_inet_ntop failed\n");
			exit(-1);
		}
	}
	event_base_dispatch(base);
}
