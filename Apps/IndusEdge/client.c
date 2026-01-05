#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sig.h"
#include "em_registers.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#define FIFO_NAME "myfifo"

#define PORT_NUM 2000

int main(int argc, char *argv[])
{
    int fd;
    struct em_registers reg;
	// TODO 4: Uncomment below lines
    int sock_fd;
    char *ip_addr;
    struct sockaddr_in my_addr;
    socklen_t my_addr_len = sizeof(my_addr);

    if (argc != 2)
    {
        printf("Usage: %s <server ip addr>\n", argv[0]);
        return 1;
    }

    ip_addr = argv[1];
	// TODO 5: Create the socket using the socket API & comment out the FIFO
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0 /* IPPROTO_TCP */)) == -1)
    {
        perror("socket");
        return 2;
    }
#if 0
    // TODO 1: Create the FIFO
    if (mknod(FIFO_NAME, S_IFIFO | S_IRUSR | S_IWUSR, 0) == -1)
    {
        perror("mknod");
    }

    printf("Waiting for writers ...\n");
#endif

    // TODO 6: Initialize the required fields for my_addr & connecto the socket
	// Comment out the FIFO related stuff
    my_addr.sin_family = AF_INET; // address family 
    my_addr.sin_port = htons(PORT_NUM); // short, network byte order
    my_addr.sin_addr.s_addr = inet_addr(ip_addr);
    bzero(&(my_addr.sin_zero), 8);

    printf("Connecting socket to %s ... ", ip_addr);
    if (connect(sock_fd, (struct sockaddr *)&my_addr, my_addr_len) == -1)
    {
        perror("connect");
        close(sock_fd);
        return 4;
    }
    printf("Done\n");
#if 0
    // TODO 2: Open the FIFO
    if ((fd = open(FIFO_NAME, O_RDONLY)) == -1)
    {
        perror("open");
        return -1;
    }
 
    printf("Got a writer:\n");
#endif
	// TODO 7: Receive the data over socket instead of FIFO
	//TODO 3: Keep on reading the EM Registers (struct em_registers) from FIFO
    while ((recv(sock_fd, &reg, sizeof(struct em_registers), 0)) > 0)
	//while ((read(fd, &reg, sizeof(struct em_registers))) > 0)
    {
        printf("Va = %u, Vb = %u Vc = %u\n", reg.va, reg.vb, reg.vc);
        printf("Va Max = %u, Vb Max = %u Vc Max = %u\n", reg.va_max, reg.vb_max, reg.vc_max);
    }
	// TODO 8: Remove this   
    close(fd);

    return 0;
}
