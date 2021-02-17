#define DEFAULT_SOURCE_
#define D_BSD_SOURCE
#define D_SVID_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <netdb.h>


void write_socket(int sockfd, unsigned int buffer_size, void* buffer) {
    unsigned int totalsent = 0;
    unsigned int not_written = buffer_size;
    int nsent = -1;

    while (not_written > 0)
    {
        nsent = write(sockfd, buffer + totalsent, buffer_size);
        if (nsent < 0) {
            if (errno == ETIMEDOUT ||
                errno == ECONNRESET ||
                errno == EPIPE)
            {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            continue;
        }
        totalsent += nsent;
        not_written -= nsent;
    }   
}

void read_socket(int sockfd, unsigned int buffer_size, void* buffer) {
    int bytes_read = 0;
    unsigned int totalsent = 0;
    unsigned int not_written = buffer_size;
    while (not_written > 0)
    {
        bytes_read = read(sockfd, buffer, buffer_size);
        if (bytes_read <= 0) {
            if (errno == ETIMEDOUT ||
                errno == ECONNRESET ||
                errno == EPIPE)
            {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            continue;
        }
        totalsent += bytes_read;
        not_written -= bytes_read;
    }
}

int main(int argc, char* argv[])
{
    int  sockfd = -1;
    char* buff;
    struct sockaddr_in serv_addr;
    FILE* file;
    unsigned int size;

    //===   validating number of arguments  =================================
    if (argc != 4) {
        printf("need 3 arguments");
        exit(EXIT_FAILURE);
    }

    //===   reading from file   =============================================
    file = fopen(argv[3], "r");
    if (file == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
        
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    rewind(file);
    buff = (char*)malloc(sizeof(char) * size);

    if (fread(buff, sizeof(char),size, file) <= 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(file);

    //===   creating socket =================================================
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }      

    //===   connecting to server    =========================================
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2])); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);// inet_aton() doesnt compile
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    //===   chatting with server    ========================================
    unsigned int n_size = htonl(size);
    write_socket(sockfd, sizeof(unsigned int), &n_size); 
    write_socket(sockfd, size, buff);     
    n_size = 0;
    read_socket(sockfd, sizeof(unsigned int), &n_size);
    
    //===   terminating session ============================================
    printf("# of printable characters: %u\n", ntohl(n_size));
    
    free(buff);
    close(sockfd);

    return 0;
}

