
#include "gfclient-student.h"

#define BUFSIZE 630


struct gfcrequest_t{
    const char *server;
    const char *path;
    unsigned short port;

    void (*writefunc)();
    void *writearg;

    gfstatus_t status;

    size_t bytesreceived;
    size_t filelen;

};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr){
    free(*gfr);
    *gfr = NULL;
    gfr = NULL;
}

gfcrequest_t *gfc_create(){
    // dummy for now - need to fill this part in

    gfcrequest_t *gfc_request;
    // initialize
    if((gfc_request = malloc(sizeof(struct gfcrequest_t))) != NULL) {        
        gfc_request->port = 0;
        gfc_request->writefunc = NULL;
        gfc_request->writearg = NULL;

        gfc_request->status = GF_ERROR;

        gfc_request->bytesreceived = 0;
        gfc_request->filelen = 0;
        
    }    
    return gfc_request;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    // // not yet implemented    
    return (*gfr)->bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    // not yet implemented    
    return (*gfr)->filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    // not yet implemented    
    return (*gfr)->status;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t **gfr){
    
    // struct sockaddr_in serv_addr;
    // struct hostent* host_info;

    struct addrinfo *srvinfo;
    struct addrinfo hints;

    char portstr[10] = {0};

    // long host_address;
    char buffer_send[BUFSIZE] = {0};
    char buffer_recv[BUFSIZE] = {0};
    char message_recv[BUFSIZE] = {0};
    char message_recv_copy[BUFSIZE];

    int bytes_recv = 0;
	int bytes_sum = 0;
    int content_bytes_sum = 0;

    char *header_recv;
    char *scheme_recv;
    char *status_recv;
    int header_len = 0;

    char *file_content;
    char *flen = NULL;

    // init socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    sprintf(portstr, "%d", (*gfr)->port);
    if(getaddrinfo((*gfr)->server, portstr, &hints, &srvinfo) !=0){
        perror("getaddrinfo");
    }
    printf("init socket\n");
    int sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol);
    
    // connect
    printf("connect\n");
    if(connect(sockfd, srvinfo->ai_addr, srvinfo->ai_addrlen) < 0){
        (*gfr)->status = GF_ERROR;
        close(sockfd);
        freeaddrinfo(srvinfo);
        return -1;
    }
    freeaddrinfo(srvinfo);

    // buid request message
    printf("build message\n");
    strcat(buffer_send, "GETFILE GET ");
    strcat(buffer_send, (*gfr)->path);
    strcat(buffer_send, "\r\n\r\n");

    // send request message header
    printf("send message: %s\n", buffer_send);
    send(sockfd, buffer_send, strlen(buffer_send), 0);

    while(strstr(message_recv, "\r\n\r\n") == NULL){
        if (strlen(message_recv) > 200){
            // no terminator in header
            (*gfr)->status = GF_INVALID;
			close(sockfd);
			return -1;
        }
        // bytes_recv = recv(sockfd, buffer_recv, BUFSIZE-1, 0);
        bytes_recv = read(sockfd, buffer_recv, BUFSIZE-1);
        // printf("%d", bytes_recv);
        bytes_sum = bytes_sum + bytes_recv;        
        if(bytes_recv <= 0){
            // prematurely close connection
            (*gfr)->status = GF_INVALID;
            close(sockfd);
            return -1;
        }
        // printf("buffer recv: %s, bytes recv: %d\n", buffer_recv, bytes_recv);
        buffer_recv[bytes_recv] = '\0';
        strcat(message_recv, buffer_recv);
        // buffer_recv[0] = '\0';
    }
    // message_recv[bytes_sum] = '\0';
    strcpy(message_recv_copy, message_recv);

    // receive statistics
	printf("bytes sum: %d\n", bytes_sum);
	printf("message received: %s\n", message_recv);
    
    // split message received
    header_recv = strtok(message_recv, "\r\n");
    header_len = strlen(header_recv);
    scheme_recv = strtok(header_recv, " ");
    status_recv = strtok(NULL, " ");    

    // get file content
    if (bytes_sum > header_len + 4){
        file_content = header_recv + header_len + 4;
        content_bytes_sum = bytes_sum - header_len - 4;
        printf("content in header");
    }
    else{
        file_content = NULL;
        content_bytes_sum = 0;
        printf("no content in header");
    }

    printf("header: %s\n", header_recv);
	printf("header length: %d\n", header_len);
	printf("scheme: %s\n", scheme_recv);
    printf("status: %s\n", status_recv);
    printf("content bytes: %d\n", content_bytes_sum);

    // handle header issues
    if (scheme_recv == NULL || status_recv == NULL){
        (*gfr)->status = GF_INVALID;
        close(sockfd);
        return -1;
    }

    if (!strcmp(status_recv, "OK")){
        strtok(message_recv_copy, " ");
        strtok(NULL, " ");
        flen = strtok(NULL, "\r\n\r\n");
        (*gfr)->filelen = atoi(flen);
        printf("file length: %s\n", flen);
    }

    // status check
    if (strcmp(scheme_recv, "GETFILE") != 0){
        // unknow scheme
        printf("sheme error %s\n", scheme_recv);
        (*gfr)->status = GF_INVALID;
        close(sockfd);
        return -1;
    }
    // else if((strcmp(status_recv, "OK") && strcmp(status_recv, "ERROR") && strcmp(status_recv, "FILE_NOT_FOUND") && strcmp(status_recv, "INVALID"))){
    //     // unknown status
    //     printf("unknown status error %s\n", status_recv);
    //     (*gfr)->status = GF_INVALID;
    //     close(sockfd);
    //     return -1;
    // }
    else if((!strcmp(status_recv, "FILE_NOT_FOUND") || !strcmp(status_recv, "ERROR")) && flen != NULL){
        // file length valid, but FILE_NOT_FOUND
        (*gfr)->status = GF_INVALID;
        close(sockfd);
        return -1;
    }
    else if(!strcmp(status_recv, "ERROR")){
        printf("status error %s\n", status_recv);
        (*gfr)->status = GF_ERROR;
        close(sockfd);
        return 0;
    }
    else if(!strcmp(status_recv, "FILE_NOT_FOUND")){
        printf("status error %s\n", status_recv);
        (*gfr)->status = GF_FILE_NOT_FOUND;
        close(sockfd);
        return 0;
    }    
    else if(!strcmp(status_recv, "INVALID")){
        printf("status error %s\n", status_recv);
        (*gfr)->status = GF_INVALID;
        close(sockfd);
        return -1;
    }
    else if(!strcmp(status_recv, "OK")) {
        printf("status ok %s\n", status_recv);
    }
    else{
        printf("status error %s\n", status_recv);
        (*gfr)->status = GF_INVALID;
        close(sockfd);
        return 0;
    }
    
    // write file
    printf("write file\n");
    if (file_content != NULL){
        (*gfr)->writefunc((void *) file_content, content_bytes_sum, (*gfr)->writearg);

        if (content_bytes_sum >= (*gfr)->filelen){
            printf("all content in header");
            (*gfr)->status = GF_OK;
            (*gfr)->bytesreceived = content_bytes_sum;
            close(sockfd);
            return 0;
        }
    }
    printf("write file finish\n");

    // receive body
    while(1){
        bytes_recv = recv(sockfd, buffer_recv, BUFSIZE-1, 0);               
        buffer_recv[bytes_recv] = '\0';
        if(bytes_recv<=0 && content_bytes_sum < (*gfr)->filelen){
            // prematurely close connection
            (*gfr)->writefunc(buffer_recv, bytes_recv, (*gfr)->writearg);
			fprintf(stderr, "\nERROR: Communication Timeout\n");
			(*gfr)->status = GF_OK;
            (*gfr)->bytesreceived = content_bytes_sum;
			close(sockfd);
			return -1;
        }
        if(bytes_recv>0){
            content_bytes_sum = content_bytes_sum + bytes_recv;    
        }  
        if (content_bytes_sum >= (*gfr)->filelen){
            (*gfr)->writefunc(buffer_recv, bytes_recv, (*gfr)->writearg);
            break;
        }

        // normal write
        (*gfr)->writefunc(buffer_recv, bytes_recv, (*gfr)->writearg);        
    }    

    printf("receive all\n");
    (*gfr)->status = GF_OK;
    (*gfr)->bytesreceived = content_bytes_sum;
    close(sockfd);
    return 0;

}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg){

}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){

}

void gfc_set_path(gfcrequest_t **gfr, const char* path){
    (*gfr)->path = path;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*gfr)->port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char* server){
    (*gfr)->server = server;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*gfr)->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    (*gfr)->writefunc = writefunc;
}

const char* gfc_strstatus(gfstatus_t status){
    const char *strstatus = NULL;

    switch (status)
    {
        default: {
            strstatus = "UNKNOWN";
        }
        break;

        case GF_INVALID: {
            strstatus = "INVALID";
        }
        break;

        case GF_FILE_NOT_FOUND: {
            strstatus = "FILE_NOT_FOUND";
        }
        break;

        case GF_ERROR: {
            strstatus = "ERROR";
        }
        break;

        case GF_OK: {
            strstatus = "OK";
        }
        break;
        
    }

    return strstatus;
}

