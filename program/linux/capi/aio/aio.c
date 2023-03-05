#define _GNU_SOURCE // O_DIRECT undeclared, You should define _GNU_SOURCE before including <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>
#include <errno.h>

int io_setup(unsigned nr, aio_context_t* ctxp)
{
	return syscall(SYS_io_setup, nr, ctxp);
}

int io_destroy(aio_context_t ctx)
{
	return syscall(SYS_io_destroy, ctx);
}

int io_submit(aio_context_t ctx, long nr, struct iocb** iocbpp)
{
	return syscall(SYS_io_submit, ctx, nr, iocbpp);
}

int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
	struct io_event* events, struct timespec* timeout)
{
	return syscall(SYS_io_getevents, ctx, min_nr, max_nr, timeout);
}

#define MAX_IO_NUM 10

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("param less 3\n");
		exit(1);
	}
	char* data = NULL;
	posix_memalign((void**)&data, sysconf(_SC_PAGESIZE), 1024);
	strncpy(data, argv[1], 1024);
	const char* filename = argv[2];

	printf("get data %s to %s\n", data, filename);

	int fd = -1;
	aio_context_t ctx;
	struct iocb io;
	struct iocb* pio[1] = { &io };
	struct io_event events[MAX_IO_NUM];
	struct timespec timeout;

	memset(&ctx, 0, sizeof(ctx));
	if (io_setup(MAX_IO_NUM, &ctx))
	{
		printf("io setup error\n");
		exit(1);
	}

	fd = open(filename, O_CREAT|O_RDWR|O_DIRECT, S_IRWXU|S_IRWXG|S_IROTH);
	if (fd < 0)
	{
		perror("open error");
		io_destroy(ctx);
		exit(1);
	}

	memset(events, 0, sizeof(events));
	memset(&io, 0, sizeof(io));
	io.aio_data = (uint64_t)data;
	io.aio_fildes = fd;
	io.aio_lio_opcode = IOCB_CMD_PWRITE;
	io.aio_reqprio = 0;
	
	io.aio_buf = data;
	io.aio_nbytes = 512;
	io.aio_offset = 0;

	if (io_submit(ctx, 1, pio) != 1)
	{
		close(fd);
		io_destroy(ctx);
		perror("io submit error");
		exit(1);
	}

	timeout.tv_sec = 2;
	timeout.tv_nsec = 0;

	int count = 1;
	while (1)
	{
		printf("start get events %d times\n", count++);
		int ret = io_getevents(ctx, 1, MAX_IO_NUM, events, &timeout);
		printf("ret = %d\n", ret);
		if (ret == 1)
		{
			close(fd);
			printf("get it\n");
			break;
		}
		else if (ret == -1)
		{
			printf("errno = %d \n", errno);
			perror("io getevents error");
		}
		sleep(1);
	}

	io_destroy(ctx);

	return 0;
}

