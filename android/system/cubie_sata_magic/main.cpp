#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "cubie"

#include "cutils/log.h"

#define CUBIE_MAGIC 0xcb525000

struct cubie_hdr {
	uint32_t cubie_magic;
	int8_t cubie_token[32];
	uint32_t reserved[4];
	uint32_t csum;
};

int get_sata_devname(char *name_buf, int len)
{
	int i, n;
	char namebuf[128];
	char path[128];


	for (i = 'a'; i <= 'z'; i++ ){
		memset(namebuf, sizeof(namebuf), 0);
		snprintf(namebuf, sizeof(namebuf), "/sys/block/sd%c", i);

		if (access(namebuf, O_RDONLY)) {
			continue;
		}

		n = readlink(namebuf, path, sizeof(path) - 1);
		if (n < 0) {
			continue;
		}

		ALOGI("%s -> %s\n", namebuf, path);
		if (strstr(path, "sw_ahci") && strstr(path, "ata")) {
			memset(name_buf, 0, len);
			strncpy(name_buf, namebuf, len);
			name_buf[1] = 'd';
			name_buf[2] = 'e';
			name_buf[3] = 'v';

			return 1;
		}


	}

	return 0;
}

uint32_t calc_csum(struct cubie_hdr *hdr)
{
	uint32_t csum, i;
	int y = sizeof(struct cubie_hdr) / 4 - 1;
	uint32_t *p = (uint32_t *)hdr;

	csum = CUBIE_MAGIC;

	for (i=0; i<y; i++) {
		csum += p[i];
	}

	return csum;
}

int write_cubie_header(const char *devpath)
{
	struct cubie_hdr hdr;
	uint32_t csum;
	int fd;
	int ret;

	memset(&hdr, 0, sizeof(hdr));
	hdr.cubie_magic = CUBIE_MAGIC;
	snprintf((char *)hdr.cubie_token, sizeof(hdr.cubie_token), "%s", "cubiesata");

	csum = calc_csum(&hdr);
	hdr.csum = csum;



//	if (access(devpath, W_OK)) {
//		ALOGW("Permission denied for %s", devpath);
//		return 0;
//	}

	fd = open(devpath, O_RDWR);
	if (fd < 0) {
		ALOGW("Open failed: %s", devpath);
		return 0;
	}

	ret = write(fd, &hdr, sizeof(hdr));

	if (ret < 0) {
		ALOGW("Write failed: %s", devpath);
		close(fd);
		return 0;
	}

	close(fd);
	sync();

	return 1;
}


int delete_cubie_header(const char *devpath)
{
	struct cubie_hdr hdr;
	uint32_t csum;
	int fd;
	int ret;

	memset(&hdr, 0, sizeof(hdr));
       	printf("sata magic delete------\n");
       /*	if (access(devpath, W_OK)) {
		ALOGW("Permission denied for %s", devpath);
		return 0;
	}*/

	fd = open(devpath, O_RDWR);
	if (fd < 0) {
		ALOGW("Open failed: %s", devpath);
		return 0;
	}

	ret = write(fd, &hdr, sizeof(hdr));

	if (ret < 0) {
		ALOGW("Write failed: %s", devpath);
		close(fd);
		return 0;
	}

	close(fd);
	sync();

	return 1;
}

int report_cubie_header(const char *devpath)
{
	struct cubie_hdr hdr;
	uint32_t csum;
	int fd;
	int ret;

	memset(&hdr, 0, sizeof(hdr));

	if (access(devpath, O_RDONLY)) {
		ALOGW("Permission denied for %s", devpath);
		return 0;
	}

	fd = open(devpath, O_RDONLY);
	if (fd < 0) {
		ALOGW("Open failed: %s", devpath);
		return 0;
	}

	ret = read(fd, &hdr, sizeof(hdr));

	if (ret < 0) {
		ALOGW("read failed: %s", devpath);
		close(fd);
		return 0;
	}

	printf("magic: %08x\n", hdr.cubie_magic);
	printf("csum: %08x\n", hdr.csum);

	close(fd);

	return 1;
}

int check_cubie_header(const char *devpath)
{
	struct cubie_hdr hdr;
	uint32_t csum;
	int fd;
	int ret;

	memset(&hdr, 0, sizeof(hdr));

	if (access(devpath, O_RDONLY)) {
		ALOGW("Permission denied for %s", devpath);
		return 0;
	}

	fd = open(devpath, O_RDONLY);
	if (fd < 0) {
		ALOGW("Open failed: %s", devpath);
		return 0;
	}

	ret = read(fd, &hdr, sizeof(hdr));

	if (ret < 0) {
		ALOGW("read failed: %s", devpath);
		close(fd);
		return 0;
	}

	close(fd);

	if (hdr.csum == calc_csum(&hdr)) {
		printf("check ok\n");
	} else {
		printf("check failed\n");
	}

	return 1;

}



int main(int argc, char *argv[])
{
	int opt = -1;
	char devname[64];

	if (!get_sata_devname(devname, sizeof(devname))) {
		return -1;
	}

	while ((opt = getopt(argc, argv, "wrcdh")) != -1 ) {
		switch (opt) {
		case 'w':
			if (!write_cubie_header(devname)) {
				ALOGW("failed to write cubie header\n");
				return -1;
			}
			break;
		case 'r':
			if (!report_cubie_header(devname)) {
				ALOGW("failed to write cubie header\n");
				return -1;
			}
			break;
		case 'c':
			if (!check_cubie_header(devname)) {
				ALOGW("failed to write cubie header\n");
				return -1;
			}
			break;
		case 'd':
			if (!delete_cubie_header(devname)) {
				ALOGW("failed to write cubie header\n");
				return -1;
			}
			break;
		default:
			break;
		}
	}

	return 0;
}
