#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sig.h"
#include "cmd.h"
#include "em_registers.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#define FIFO_NAME "myfifo"

#define PORT_NUM 1000

int main(int argc, char *argv[])
{
    int fd, choice;
    struct em_registers reg;
	// TODO 4: Uncomment below lines
    int sock_fd;
    char *ip_addr;
    struct sockaddr_in my_addr;
    socklen_t my_addr_len = sizeof(my_addr);
	struct cmd cmd_buf;
	struct resp resp_buf;

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
	while (1)
	{
		printf("1: Set the Outputs\n");
		printf("2: Get the Outputs\n");
		printf("0: Exit!!\n");
		scanf("%d", &choice);
		
		if (choice == 1)
		{
			printf("Which outputs do you want to set?\n");
			printf("Enter the value between 0 to 15\n");
			printf("0 means all low, 1 (0001) means first o/p high and so on\n");
			scanf("%d", &choice);
			if (!(choice >= 0 && choice <= 15))
			{
				printf("Invalid value\n");
				continue;
			}
			cmd_buf.cmd_no = CMD_SET_OP;
			cmd_buf.cmd_data = choice;
			if (send(sock_fd, &cmd_buf, sizeof(cmd_buf), 0) != sizeof(cmd_buf))
			{
				printf("Sending command failed\n");
				continue;
			}
			if (recv(sock_fd, &cmd_buf, sizeof(resp_buf), 0) != sizeof(resp_buf))
			{
				printf("Receiving command failed\n");
				continue;
			}
		}
		else if (choice == 0)
			break;
	}
	
    close(sock_fd);

    return 0;
}
