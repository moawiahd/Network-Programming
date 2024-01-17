//Moawiah AL DOUM 139912
//Ramzi Mohammed 123844

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/select.h>
#include <dirent.h>
#include <signal.h>
#define MAXLINE 1024
#define SA struct sockaddr

void connectToServer(int sockfd, char message[MAXLINE], struct sockaddr_in servaddr, socklen_t len)
{
    // send message message to server
    if (sendto(sockfd, message, MAXLINE, 0, (SA *)&servaddr, len) < 0)
    {printf("!!! Error To Send !!!\n");exit(1);}
    
    if (strcmp(message, "END-UDP") == 0)
    {return;}
    
    char tmp1;

    if ((recvfrom(sockfd, &tmp1, 1, 0, (SA *)&servaddr, &len)) < 0)
    {printf("Error Receiving byte .");exit(1);}


    if (tmp1 != 0x55)
    {exit(1);}
    
    char tmpAA = 0xAA;

    if (sendto(sockfd, &tmpAA, sizeof(tmpAA), 0, (SA *)&servaddr, len) < 0)
    {printf("!!! Error In Sending !!!\n");exit(1);}
}


void getFilesFromServer(int sockfd, struct sockaddr_in servaddr, socklen_t len)
{
    char buffer[MAXLINE];
    if (recvfrom(sockfd, buffer, MAXLINE, 0, (SA *)&servaddr, &len) < 0)
    {printf("Error in recvfrom !! .");exit(1);}
    printf("%s", buffer);
}

void getFiles(int sockfd, struct sockaddr_in servaddr, socklen_t len)
{
    char file[MAXLINE];

    // get file name
    bzero(file, MAXLINE);
    printf("Please Enter the file name to request  : ");
    scanf("%s", file);
    char newFileName[MAXLINE];

    // get new name for the file
    printf("Enter The NewFile Name -->");
    scanf("%s", newFileName);

    // send file name
    if (sendto(sockfd, file, MAXLINE, 0, (SA *)&servaddr, len) < 0)
    {printf("Errorsending file name to the server \n");return;}
    
    // if 'done' then exit
    if (!strcmp(file, "done"))
        return;

    char tmp;

    // get confirmation that the file exists
    recvfrom(sockfd, &tmp, 1, 0, (SA *)&servaddr, &len);

    if (tmp == 0)
    {printf("File is not exist\n");return;}

    // create file
    FILE *newFile = fopen(newFileName, "wb");
    while (5)
    {
        int bytes;
        char chunk[MAXLINE];
        bytes = recvfrom(sockfd, chunk, MAXLINE, 0, (SA *)&servaddr, &len);
        if (bytes <= 0)
            break;

        // write to the file
        fwrite(chunk, 1, bytes, newFile);
    }
}

void udp(int sockfd, struct sockaddr_in servaddr, socklen_t len, char m[MAXLINE])
{

    // connect to server
    connectToServer(sockfd, m, servaddr, len);
    if (strcmp(m, "END-UDP") == 0)
    {
        return;
    }
    // get files from the server
    getFilesFromServer(sockfd, servaddr, len);

    // get files from the server for downloading
    getFiles(sockfd, servaddr, len);
}

void tcp(int port, char server_ip[16], int flag)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    // assign family, IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton error");
        exit(1);
    }
    socklen_t len = sizeof(servaddr);

    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0)
    {printf("Can't Creat TCP Socket\n");exit(1);}
    
    if (connect(connfd, (SA *)&servaddr, len) < 0)
    {printf("Failed To Connect To TCP Server\n");exit(1);}
    
    if (flag == 0)
    {
        char end[MAXLINE] = "END-TCP";
        write(connfd, end, MAXLINE);
        close(connfd);
        return;
    }
    DIR *directory;
    struct dirent *entry;
    directory = opendir(".");
    if (directory == NULL)
    {
        perror("Failed To Open The Directory");
        exit(EXIT_FAILURE);
    }
    char cd_files[MAXLINE];
    bzero(cd_files, MAXLINE);
    while ((entry = readdir(directory)) != NULL)
    {
        strcat(cd_files, entry->d_name);
        strcat(cd_files, "\n");
    }
    closedir(directory);

    printf("%s", cd_files);
    printf("File To Uploud : \n");
    char c[MAXLINE];
    bzero(c, MAXLINE);
    sleep(0.5);
    fd_set rest;
    for (;;)
    {
        FD_SET(fileno(stdin), &rest);
        FD_SET(connfd, &rest);
        int maxfd = (connfd > fileno(stdin) ? connfd : fileno(stdin)) + 1;
        select(maxfd, &rest, NULL, NULL, NULL);
        if (FD_ISSET(fileno(stdin), &rest))
        {
            scanf("%s", c);
            break;
        }
        if (FD_ISSET(connfd, &rest))
        {
            char tmp[MAXLINE];
            read(connfd, tmp, MAXLINE);
        }
    }

    FILE *f = fopen(c, "rb");
    if (f == NULL)
    {
        printf("sorry file %s not found  !! ...\n", c);
        char t[MAXLINE] = "NOT FOUND";
        write(connfd, t, MAXLINE);
        close(connfd);
        return;
    }
    else
        write(connfd, &c, MAXLINE);
    char resp = 0;

    read(connfd, &resp, 1);
    if (resp != 1)
    {
        printf("Server Error\n");
        return;
    }

    char buffer[MAXLINE];
    bzero(buffer, MAXLINE);
    int bytes;
    while ((bytes = fread(buffer, 1, MAXLINE, f)) > 0)
    {
        write(connfd, &buffer, bytes);
    }
    write(connfd, NULL, 0);
    printf("File Sent Successfully\n");
    close(connfd);
}

char ip[16];
int port1, sockudp;

void sig(int signo)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port1);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);

    socklen_t len = sizeof(servaddr);
    char message[MAXLINE] = "END-UDP";
    udp(sockudp, servaddr, len, message);
    int x = 0;
    tcp(port1, ip, x);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <Host> <Port-Number>\n", argv[0]);
        exit(1);
    }

    // get ip address of HOST
    struct hostent *hptr;
    char *ptr;
    ptr = *(argv + 1);
    if ((hptr = gethostbyname(ptr)) == NULL)
    {
        printf("domain name %s not Found sorry \n", argv[1]);
        exit(1);
    }
    char server_ip[16];
    inet_ntop(hptr->h_addrtype, hptr->h_addr_list[0], server_ip, sizeof(server_ip));
    strcpy(ip, server_ip);
    int sockfd;
    // socket create and verification
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {printf("--!!! Error In Creation Socket !!!--\n");exit(0);}
    
    sockudp = sockfd;
    // check if port is valid
    int port = atoi(argv[2]);
    if (port < 0 || port > 65535)
    {
        printf("Invalid port number %d\n", port);
        exit(0);
    }
    port1 = port;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    // assign family, IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton error");
        exit(1);
    }
    socklen_t len = sizeof(servaddr);
    signal(SIGQUIT, sig);
    while (1)
    {   printf("\n**********welcome to NES 416 HW #5 *****************\n");
        printf("1- Downloading File via UDP\n");
        printf("2- Uploading file via TCP\n");
        printf("3- Insert SIGQUIT to quit\n");
        printf("****************************************************\n\n");
        int x;
        printf("---->Please Enter Your Choice<----\n");
        scanf("%d", &x);
        if (x == 1)
        {   
            char message[MAXLINE] = "hello";
            udp(sockfd, servaddr, len, message);
        }
        else if (x == 2)
        {
            int x = 1;
            tcp(port, server_ip, x);
        }
        else if (x == 3)
        {
            char message[MAXLINE] = "END-UDP";
            udp(sockfd, servaddr, len, message);
            int x = 0;
            tcp(port, server_ip, x);
            exit(0);
        }
        else
        {
            printf("* * * Wrong choice sorry again please * * * \n");
        }
    }
}
