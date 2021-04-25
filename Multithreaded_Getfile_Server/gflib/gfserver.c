
#include "gfserver-student.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */
#define BUFSIZE 630

struct gfcontext_t{
    int socket_id;
    char *path;
};

struct gfserver_t{
        
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
    unsigned short port;
    int maxpending;
    void *handlerarg;

};

void gfs_abort(gfcontext_t **ctx){
    close((*ctx)->socket_id);
}

gfserver_t* gfserver_create(){
    gfserver_t *gfc_server;
    // initialize
    if((gfc_server = malloc(sizeof(struct gfserver_t))) != NULL) {        
        gfc_server->handler = NULL;
        gfc_server->handlerarg = NULL;
        
    }    
    return gfc_server;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    char *data_char = (char *) data;
    char message_send[BUFSIZE] = {0};
    int message_send_size = 0;
    int chunks = (len/BUFSIZE) + 1;

    printf("send data len: %d, chunks: %d\n", (int) len, chunks);
    for(int send_count=0; send_count<chunks; send_count++){
        if(send_count == (chunks-1)){
            printf("send final\n");
            memcpy(message_send, &data_char[send_count*BUFSIZE], (len - (send_count*BUFSIZE)));
            message_send_size = len - (send_count * BUFSIZE);
        }
        else{
            memcpy(message_send, &data_char[send_count*BUFSIZE], BUFSIZE);
            message_send_size = BUFSIZE;
        }

        if(send((*ctx)->socket_id, message_send, message_send_size, 0) < 0){
            printf("error: data not finish\n");
            close((*ctx)->socket_id);
            return -1;
        }
        memset(message_send,'\0',sizeof(message_send));

    }
    printf("send finish\n");

    return len;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    char strstatus[BUFSIZE] = {0};
    char message_header[BUFSIZE] = {0};
    char str_filelen[BUFSIZE] = {0};

    snprintf(str_filelen, 10, "%d", ((int) file_len));
    
    if(status == GF_FILE_NOT_FOUND){
        printf("send header: file not found\n");
        strcat(strstatus, "FILE_NOT_FOUND");
    }
    else if(status == GF_INVALID){
		printf("send header: Invalid\n");
		strcat(strstatus, "INVALID");
	}
	else if(status == GF_ERROR){
		printf("send header: Error\n");
		strcat(strstatus, "ERROR");
	}
	else if(status == GF_OK){
		printf("send header: OK\n");
		strcat(strstatus, "OK");
	}
	else{
		printf("\nERROR: Incorrect header status\n");
		return -1;
	}

    strcat(message_header, "GETFILE ");
	strcat(message_header, strstatus);

    if(!strcmp(strstatus, "OK"))
	{
		strcat(message_header, " ");
		strcat(message_header, str_filelen);
	}
	
	strcat(message_header, "\r\n\r\n");
    printf("send header: %s\n", message_header);
    send((*ctx)->socket_id, message_header, strlen(message_header), 0);
    return 0;

}

void gfserver_serve(gfserver_t **gfs){

    int sockfd, newsockfd, clilen;
    // char buffer_send[BUFSIZE] = {0};
    char buffer_recv[BUFSIZE] = {0};
    // char message_send[BUFSIZE] = {0};
    char message_recv[BUFSIZE] = {0};
    // char message_recv_copy[BUFSIZE] = {0};
    int one = 1;

    int bytes_recv = 0;
	// int bytes_send = 0;

    struct sockaddr_in serv_addr, cli_addr;

    char *header_recv;
    char *scheme_recv;
    char *method_recv;
    char *path_recv;
    int header_len = 0;
    char server_path[BUFSIZE] = "server_root";

    // char *header_send;
    // char *scheme_send;
    // char *status_send;

    gfcontext_t *ctx;
    ctx = malloc(sizeof(gfcontext_t));

    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((*gfs)->port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
    bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));  
    // listen(sockfd, (*gfs)->maxpending);
    
    // start listening
    for(;;)
    {   
        listen(sockfd, (*gfs)->maxpending);
        memset(message_recv,'\0',sizeof(message_recv));

        printf("start accept\n");
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);        
        ctx->socket_id = newsockfd;
        bzero(buffer_recv, sizeof(buffer_recv));

        printf("read recv\n");        
        while(strstr(message_recv, "\r\n\r\n") == NULL)
		{
			bytes_recv = recv(newsockfd, buffer_recv, sizeof(buffer_recv), 0);
			if(bytes_recv == -1)
			{				
				break;
			}
			buffer_recv[bytes_recv] = '\0';
			strcat(message_recv, buffer_recv);			
		}
        printf("message recv: %s\n", message_recv);
        if(bytes_recv == -1)
		{            
			fprintf(stderr, "recv error\n");
			close(sockfd);
            // break;
			continue;
		}

        // strcpy(message_recv_copy, message_recv);

        printf("recv success\n");

        header_recv = strtok(message_recv, "\r\n");
        header_len = strlen(header_recv);
        scheme_recv = strtok(header_recv, " ");
        method_recv = strtok(NULL, " ");
        path_recv = strtok(NULL, "\r\n\r\n");

        printf("header: %s\n", header_recv);
        printf("header length: %d\n", header_len);
        printf("scheme: %s\n", scheme_recv);
        printf("method: %s\n", method_recv);
        printf("path: %s\n", path_recv);

        if(scheme_recv == NULL || method_recv == NULL || path_recv == NULL){
            printf("header null\n");
            gfs_sendheader(&ctx, GF_INVALID, 0);
            close(newsockfd);
            continue;
        }
        else if(strcmp(scheme_recv, "GETFILE") || strcmp(method_recv, "GET") || strncmp(path_recv, "/", 1)){
            printf("header not compare\n");
            gfs_sendheader(&ctx, GF_INVALID, 0);
            close(newsockfd);
            continue;
        }

        if(path_recv !=NULL){
            strcat(server_path, path_recv);
			ctx->path = server_path;
        }
        (*gfs)->handler(&ctx, ctx->path, (*gfs)->handlerarg);

        close(newsockfd);

    }

}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*gfs)->handlerarg = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*gfs)->handler = handler;    
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*gfs)->maxpending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*gfs)->port = port;
}


