#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "sig.h"
#include "em_registers.h"
#include "sem.h"
#include "sock.h"
#include "io_ioctl.h"
#include "cmd.h"

#define FIFO_NAME "myfifo"
#define KEY 0x12345 // Would retrieve the same if not detached
#define PORT_NUM 2000
#define IO_PORT_NUM 1000

#define IO_FILE	"/dev/gpio_drv0"

void sig(int signum)
{
    printf("Received signal %d\n", signum);
}

int get_lib_count()
{
    int libcount;
    // TODO 6: Open the pipe with ls lib*.so and return the lib count
	FILE *stream = popen("ls lib*.c | wc -l", "r");

    if (stream == NULL)
    {
        perror("popen");
        return -1;
    }

    if (fscanf(stream, "%d", &libcount) > 0)
        return libcount;
    
    return -1;
}

void *shm_allocate(key_t key, size_t shm_size,
        int flags, int *shm_id, void *addr)
{
    // TODO 7: Allocate the shared memory
    *shm_id  = shmget(key, shm_size, flags);
    printf("The segment id: %d (0x%X)\n", *shm_id, *shm_id);

    // TODO 8: Attach the shared memory
    return shmat(*shm_id, addr, 0);
}

void *output_thread(void* ip_addr)
{
	int op_fd, eth_fd, sock_fd;
    char remote_ip[16];
	struct cmd cmd_buf;
	struct resp resp_buf;

	op_fd = open(IO_FILE, O_RDONLY);
	if (op_fd < 0)
	{
		perror("Opening IO File Failed:\n");
		return NULL;
	}
	if ((sock_fd = open_socket((const char *)ip_addr, IO_PORT_NUM)) == -1)
	{
		perror("Failed to open output socket:");
		return NULL;
	}
	printf("Ip Socket Opened. Waiting for clients ...\n");

	// TODO 9A: Uncomment the below for thread to run forever
	while (1)
	{
		if ((eth_fd = get_socket(sock_fd, remote_ip)) == -1)
		{
			perror("Connection failed:");
			sleep(1);
			continue;
		}
		while (read_eth(eth_fd, &cmd_buf, sizeof(cmd_buf)) > 0)
		{
			resp_buf.resp_code = RESP_SUCCESS;
			if (cmd_buf.cmd_no == CMD_SET_OP)
			{
				printf("Receive Op set command, command_data = %d\n", cmd_buf.cmd_data);
				if ((ioctl(op_fd, DRV_SET_OUTPUT, cmd_buf.cmd_data)) != 0)
				{
					perror("Setting Output Failed:\n");
					resp_buf.resp_code = RESP_FAILED;
				}
				if ((write_eth(eth_fd, &resp_buf, sizeof(resp_buf))) < 0)
				{
					perror("write the output respond failed:");
				}
			}
			if (cmd_buf.cmd_no == CMD_GET_OP)
			{
				if ((ioctl(op_fd, DRV_GET_OUTPUT, &(cmd_buf.cmd_data))) == 0)
					resp_buf.resp_data = cmd_buf.cmd_data;
				else
				{
					perror("Setting Output Failed:\n");
					resp_buf.resp_code = RESP_FAILED;
				}
				if ((write_eth(eth_fd, &resp_buf, sizeof(resp_buf))) < 0)
				{
					perror("write the output respond failed:");
				}
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
    int fd, num, libcount;
    struct em_registers reg;
    int shm_id;
    char *shm_addr = NULL;
    const int shm_size = sizeof(struct em_registers);
    int sem_id;
	// TODO 14: Declare the variables sock_fd, eth_fd, char *ip_addr, char remote_ip[16]
    int sock_fd;
    char *ip_addr;
    int eth_fd;
    char remote_ip[16];
	pthread_t ip_thread, op_thread;

	// TODO 15: Uncomment the below lines to make sure that the ip address is passed and assign argv[1] to ip_addr
	if (argc != 2)
    {
        printf("Usage: %s <ip addr>\n", argv[0]);
        return 1;
    }

    ip_addr = argv[1];

    memset(&reg, 0, sizeof(struct em_registers));
    reg.va = 440;
    reg.vb = 438;
    reg.vc = 430;
    
    // TODO 1: Register handler sig for SIGINT and SIGPIPE
    signal_register(SIGINT, sig, NULL, NULL);
    signal_register(SIGPIPE, sig, NULL, NULL);

    // TODO 2: Get the count of lib*.so files using pipe
    libcount = get_lib_count();
    if (libcount > 0)
    {
        printf("Library count = %d\n", libcount);
    }

    // TODO 9: Allocate the shared memory using shm_allocate
    shm_addr = shm_allocate(KEY, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR, &shm_id, shm_addr);
    if (shm_addr == NULL)
    {
        printf("Unable to allocate the shared memory\n");
        return -1;
    }
    printf("Shared memory attached at address %p\n", shm_addr);

    // TODO 10: Allocate the binary semaphore using binary_semaphore_allocate
    sem_id = binary_semaphore_allocate(KEY, S_IRUSR | S_IWUSR);
    printf("Semaphore created with id: %d\n", sem_id);

    // TODO 11: Initialize the binary semaphore with binary_sempahore_set
    binary_semaphore_set(sem_id);

    pthread_create(&op_thread, NULL, output_thread, ip_addr);
    //pthread_create(&ip_thread, NULL, input_thread, NULL);
    
    // TODO 3: Create the FIFO
	// TODO 16: Open the socket using open_socket, instead of creating the FIFO
    if ((sock_fd = open_socket(ip_addr, PORT_NUM)) == -1)
	{
        return -1;
	}
	printf("Socket Opened. Waiting for clients ...\n");
#if 0
    if (mknod(FIFO_NAME, S_IFIFO | S_IRUSR | S_IWUSR, 0) == -1)
    {
        perror("mknod");
    }
    // TODO 15: Remove the below print
    printf("Waiting for readers ...\n");
#endif
	// TODO 17: Replace opening FIFO with getting the client socket using get_socket assign it to eth_fd
    if ((eth_fd = get_socket(sock_fd, remote_ip)) == -1)
        return -1;
#if 0
    // TODO 4: Open the FIFO
    if ((fd = open(FIFO_NAME, O_WRONLY)) == -1)
    {
        perror("open");
        return 1;
    }
	printf("Got a reader - Sending registers\n");
#endif

    // TODO 12: Get the semaphore using binary_semaphore_get
	if ((sem_id = binary_semaphore_get(KEY, S_IRUSR | S_IWUSR)) < 0)
    {
        perror("sem_get");
        printf("2: Semaphore connect failed\n");
        /* Cleanup */
        shmdt(shm_addr);
        shmctl(shm_id, IPC_RMID, 0);
        return -1;
    }
    else
    {
        printf("2: Semaphore connected\n");
    }

    while (1) 
    {
        // TODO 13: Synchronize the access to shared memory using semaphore and
		// Copy the EM registers from the shared memory
        binary_semaphore_wait(sem_id);
        memcpy(&reg, shm_addr, sizeof(struct em_registers)); 

        binary_semaphore_post(sem_id);

		// TODO 18: Write into the socket by using write_eth instead of writing into the FIFO
		if ((write_eth(eth_fd, &reg, sizeof(struct em_registers))) < 0)
        {
            perror("write");
            return -1;
        }
#if 0
        // TODO 5: Write EM Registers (struct em_registers) to the FIFO
    	if ((num = write(fd, &reg, sizeof(struct em_registers))) == -1)
    	{
        	perror("write");
       		close(fd);
			return -1;
    	}
    	else
        	printf("Wrote %d bytes\n", num);
#endif
        
        printf("Sent shared registers\n");
        sleep(5);
    }
	// TODO 19: Comment the below line
	//close(fd);

    return 0;
}
