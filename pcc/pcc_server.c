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
#include <signal.h>
#define TBL_SIZE 95

unsigned int table[TBL_SIZE] = {0};
int sig_flag = 0;

void sig_handle() {
    sig_flag = 1;
}

void terminate() {
    for (int i = 0; i < TBL_SIZE; ++i) {
        printf("char '%c' : %u times\n", (char)(i + 32), table[i]);
    }
    exit(EXIT_SUCCESS);
}

int read_socket(int connfd, unsigned int buffer_size, void* buffer) {
    int bytes_read = 0;
    unsigned int totalsent = 0;
    unsigned int not_written = buffer_size;
    while (not_written > 0)
    {
        bytes_read = read(connfd, buffer + totalsent, buffer_size);
        if (bytes_read <= 0) {
            if (errno == ETIMEDOUT ||
                errno == ECONNRESET ||
                errno == EPIPE)
            {
                fprintf(stderr, "%s\n", strerror(errno));
                return 1;
            }
            continue;
        }
        totalsent += bytes_read;
        not_written -= bytes_read;
    }
    return 0;
}

int write_socket(int connfd, unsigned int buffer_size, void* buffer) {
    unsigned int totalsent = 0;
    unsigned int not_written = buffer_size;
    int nsent = -1;    
    while (not_written > 0)
    {
        nsent = write(connfd, buffer + totalsent, buffer_size);
        if (nsent <= 0) {
            if (errno == ETIMEDOUT ||
                errno == ECONNRESET ||
                errno == EPIPE)
            {
                fprintf(stderr, "%s\n", strerror(errno));
                return 1;
            }
            continue;
        }
        totalsent += nsent;
        not_written -= nsent;
    }
    return 0;
}

unsigned int update_table(char* buffer, unsigned int size) {
    unsigned int cnt = 0;
    for (int i = 0; i < size; ++i) {
        int c = (int)buffer[i];
        if (32 <= c && c <= 126) {
            table[c - 32]++;
            cnt++;
        }
    }
    return cnt;
}

void accept_socket(int sockfd) {
    struct sockaddr_in cli;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    int connfd;
    unsigned int size = 0;
    char* buffer;
    unsigned int n_size = 0;    

    connfd = accept(sockfd, (struct sockaddr*)&cli, &addrsize);
    if (connfd < 0) {return;}
    if (read_socket(connfd, sizeof(unsigned int), &n_size)) {return;}
    size = ntohl(n_size);
    buffer = (char*)calloc(size, sizeof(char));
    if (read_socket(connfd, size, buffer)) {return;}
    n_size = htonl(update_table(buffer,size));
    if (write_socket(connfd, sizeof(unsigned int), &n_size)) {return;}

    free(buffer);
    close(connfd);

}

int main(int argc, char* argv[]) 
{
    int sockfd;
    struct sockaddr_in servaddr;
    unsigned int port;   
    
    memset(table, 0, TBL_SIZE);
    //===   validating number of arguments  =================================
    if (argc != 2) {
        printf("need 1 argument");
        exit(EXIT_FAILURE);
    }

	port = atoi(argv[1]);

    //===   creating socket ==================================================
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
   
    memset(&servaddr, 0, sizeof(servaddr));
    // === assign IP, PORT  ==================================================       
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    //=== handling reusing same address ======================================
    int flag = 1;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // === Binding newly created socket to given IP ==========================
    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
     
    // === listen   ==========================================================
    if ((listen(sockfd, 10)) != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }   
    
    while (1) {
        //=== chatting with client  ==========================================
        signal(SIGINT, sig_handle);
        accept_socket(sockfd);
        if (sig_flag) {terminate();}
    }
    exit(EXIT_FAILURE);
}