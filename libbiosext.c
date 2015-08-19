/*
 * Copyright 2009      Luc Verhaegen <libv@skynet.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define _GNU_SOURCE		/* memmem is useful */

#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "compat.h"
#include "libbiosext.h"

void libbiosext_set_out_dir(const char *dir_path)
{
        if (dir_path != 0)
                strcpy(output_dir, dir_path);
}

unsigned char *MMapOutputFile(char *filename, int size)
{
        char full_path[1068];
	unsigned char *Buffer;
        //char *tmp;
	int fd;

        strcpy(full_path, output_dir);
        strcat(full_path, filename);

        /* all slash signs '/' in filenames will be replaced by a backslash sign '\' */
        /*tmp = full_path;
	while ((tmp = strchr(tmp, '/')) != NULL)
                tmp[0] = '\\';*/

        fd = open(full_path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
        libbiosext_log("Error: unable to open %s: %s\n\n", full_path,
			strerror(errno));
		return NULL;
	}

	/* grow file */
	if (lseek(fd, size - 1, SEEK_SET) == -1) {
        libbiosext_log("Error: Failed to grow \"%s\": %s\n", full_path,
			strerror(errno));
		close(fd);
		return NULL;
	}

	if (write(fd, "", 1) != 1) {
        libbiosext_log("Error: Failed to write to \"%s\": %s\n",
			filename, strerror(errno));
		close(fd);
		return NULL;
	}

	Buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (Buffer == ((void *)-1)) {
        libbiosext_log("Error: Failed to mmap %s: %s\n", filename,
			strerror(errno));
		close(fd);
		return NULL;
	}

	close(fd);

	return Buffer;
}

/* TODO: Make bios identification more flexible */

static struct {
	char *String1;
	char *String2;
	 Bool(*Handler) (unsigned char *Image, int ImageLength, int ImageOffset,
			 uint32_t Offset1, uint32_t Offset2);
} BIOSIdentification[] = {
	{
	"AMIBOOT ROM", "AMIBIOSC", AMI95Extract}, {
	"$ASUSAMI$", "AMIBIOSC", AMI95Extract}, {
	"AMIEBBLK", "AMIBIOSC", AMI95Extract}, {
	"Award BootBlock", "= Award Decompression Bios =", AwardExtract}, {
	"Phoenix FirstBIOS", "BCPSEGMENT", PhoenixExtract}, {
	"PhoenixBIOS 4.0", "BCPSEGMENT", PhoenixExtract}, {
	"PhoenixBIOS Version", "BCPSEGMENT", PhoenixExtract}, {
	"Phoenix ServerBIOS 3", "BCPSEGMENT", PhoenixExtract}, {
	"Phoenix TrustedCore", "BCPSEGMENT", PhoenixExtract}, {
	"Phoenix SecureCore", "BCPSEGMENT", PhoenixExtract}, {
NULL, NULL, NULL},};

int libbiosext_extract(const char *bios_rom_path)
{
	int FileLength = 0;
	uint32_t BIOSOffset = 0;
	unsigned char *BIOSImage = NULL;
	int fd;
	uint32_t Offset1, Offset2;
	int i, len;
	unsigned char *tmp;

        fd = open(bios_rom_path, O_RDONLY);
	if (fd < 0) {
        libbiosext_log("Error: Failed to open %s: %s\n", bios_rom_path,
			strerror(errno));
		return 1;
	}

	FileLength = lseek(fd, 0, SEEK_END);
	if (FileLength < 0) {
        libbiosext_log("Error: Failed to lseek \"%s\": %s\n", bios_rom_path,
			strerror(errno));
		return 1;
	}

	BIOSOffset = (0x100000 - FileLength) & 0xFFFFF;

	BIOSImage = mmap(NULL, FileLength, PROT_READ, MAP_PRIVATE, fd, 0);
	if (BIOSImage < 0) {
        libbiosext_log("Error: Failed to mmap %s: %s\n", bios_rom_path,
			strerror(errno));
		return 1;
	}

        libbiosext_log("Using file \"%s\" (%ukB)\n", bios_rom_path, FileLength >> 10);

	for (i = 0; BIOSIdentification[i].Handler; i++) {
		len = strlen(BIOSIdentification[i].String1);
		tmp =
		    memmem(BIOSImage, FileLength - len,
			   BIOSIdentification[i].String1, len);
		if (!tmp)
			continue;
		Offset1 = tmp - BIOSImage;

		len = strlen(BIOSIdentification[i].String2);
		tmp =
		    memmem(BIOSImage, FileLength - len,
			   BIOSIdentification[i].String2, len);
		if (!tmp)
			continue;
		Offset2 = tmp - BIOSImage;

		if (BIOSIdentification[i].Handler
		    (BIOSImage, FileLength, BIOSOffset, Offset1, Offset2))
			return 0;
		else
			return 1;
	}

        libbiosext_log("Error: Unable to detect BIOS Image type.\n");
	return 1;
}
