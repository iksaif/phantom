#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>

#include "error.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

const int page_size = getpagesize();

void forget(int fd, char *p, uint64_t offset, uint64_t len)
{
	if (fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset, len) == -1)
		error_exit(true, "fallocate failed");

	if (msync(&p[offset], len, MS_ASYNC) == -1)
		error_exit(true, "msync failed");
}

void punch_holes(char *filename)
{
	int fd = open(filename, O_RDWR);
	if (fd == -1)
		error_exit(true, "Cannot access file %s\n", filename);

  if (fcntl(fd, LOCK_EX))
    error_exit(true, "Cannot lock file %s\n", filename);

	struct stat st;
	if (fstat(fd, &st) == -1)
		error_exit(true, "Stat on %s failed\n", filename);

  if (!st.st_size)
    return ;

	char *data = (char *)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!data)
		error_exit(true, "mmap failed");

	if (madvise(data, st.st_size, MADV_SEQUENTIAL) == -1)
		error_exit(true, "madvise failed");

	uint64_t countPunched = 0;

	time_t last_ts = time(NULL);

	const char *const compare_buffer = (const char *)calloc(page_size, 1);

	int64_t pOffset = -1;
	for(int64_t offset=0; offset<st.st_size; offset += page_size)
	{
		int compare_size = min(page_size, st.st_size - offset);

		if (memcmp(&data[offset], compare_buffer, compare_size) != 0)
		{
			if (pOffset != -1)
			{
				uint64_t nToDo = offset - pOffset;

				forget(fd, data, pOffset, nToDo);
				countPunched += nToDo;

				pOffset = -1;
			}
		}
		else if (pOffset == -1)
		{
			pOffset = offset;

			time_t now_ts = time(NULL);

			if (now_ts - last_ts >= 2)
			{
				printf("%5.2f%% left, %5.2f%% gain\r",
					double((st.st_size - offset) * 100) / double(st.st_size),
					double(countPunched * 100) / double(st.st_size));

				fflush(NULL);

				last_ts = now_ts;
			}
		}
	}

	if (pOffset != -1)
	{
		uint64_t nToDo = st.st_size - pOffset;

		forget(fd, data, pOffset, nToDo);

		countPunched += nToDo;
	}

	if (munmap(data, st.st_size) == -1)
		error_exit(true, "munmap failed");

	close(fd);

	printf("%s: punched %ld bytes\n", filename, countPunched);
}

int main(int argc, char *argv[])
{
	if (argc > 1) {
    for (int i = 0; i < argc - 1; ++i) {
      punch_holes(argv[i]);
    }
  } else {
    char line[1024];
    int ret = 0;

    while (ret != EOF) {
      ret = scanf("%1023[^\n]%*c", line);
      punch_holes(line);
    }
  }
  return 0;
}
