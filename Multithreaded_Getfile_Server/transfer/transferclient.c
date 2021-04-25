#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define BUFSIZE 630

#define USAGE                                                  \
    "usage:\n"                                                 \
    "  transferclient [options]\n"                             \
    "options:\n"                                               \
    "  -s                  Server (Default: localhost)\n"      \
    "  -p                  Port (Default: 30605)\n"            \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 30605;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:xp:o:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    int count;
    char buffer[BUFSIZE];
    struct hostent* host_info;
    long host_address;
    struct sockaddr_in serv_addr;
    FILE *fp = fopen(filename, "wb");
    if(fp == NULL){
        printf("Cannot open file, press any key to exit!\n");        
        exit(0);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    host_info = gethostbyname(hostname);  
    memcpy(&host_address, host_info->h_addr, host_info->h_length);

    bzero((char *) &serv_addr, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = host_address;
    serv_addr.sin_port = htons(portno);

    connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    while((count = recv(sockfd, buffer, BUFSIZE, 0)) > 0 ){
        fwrite(buffer, count, 1, fp);
    }
    fclose(fp);
    close(sockfd);

}
