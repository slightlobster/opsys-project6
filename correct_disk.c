/*
CSE 30341 Spring 2025 Flash Translation Assignment.
This is the flash translation layer.
You should write all your code here.
*/

#include "disk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
Structure of the flash translation layer.
Go ahead and add or change things here as needed.
*/

struct disk {
	struct flash_drive *flash_drive;
	int nreads;
	int nwrites;
	int ndisk_blocks; // keep track of number of disk blocks, flash pages, and flash blocks for easier use later
	int nflash_pages;
	int nflash_blocks;
	int *dtf;			// disk_to_flash, dtf[8] = 12 means disk block 8 is currently in flash page 12
	int *flash_pages;	// tracks the state of each flash page
	int *flash_blocks;	// tracks the state of each flash block
};

/*
Create a new flash translation layer for this flash drive f, and simulated number of blocks
Go ahead and add or change things here as needed.
*/

// helpful enums for data structure tracking
enum {STALE = -2, EMPTY = -1}; // used for marking flash_pages state, any number >= 0 corresponds to a disk block
enum {COPY = -1, FREE = 0, USED = 1, GARBAGE = 2}; // used for marking flash_blocks state


struct disk * disk_create( struct flash_drive *f, int disk_blocks )
{
	struct disk *d = malloc(sizeof(*d));
	d->flash_drive = f;
	d->nreads = 0;
	d->nwrites = 0;
	d->ndisk_blocks = disk_blocks;
	d->nflash_pages = flash_npages(f);
	d->nflash_blocks = flash_npages(f) / flash_npages_per_block(f);
	d->dtf = malloc(disk_blocks * sizeof(int));
	// need to mark these invalid to start
	for (int i = 0; i < disk_blocks; i++) {
		d->dtf[i] = -1;
	}
	d->flash_pages = calloc(flash_npages(f), sizeof(int));
	// mark all flash pages as unused
	for (int i = 0; i < flash_npages(f); i++) {
		d->flash_pages[i] = EMPTY;
	}
	d->flash_blocks = calloc(flash_npages(f) / flash_npages_per_block(f), sizeof(int));
	d->flash_blocks[flash_npages(f) / flash_npages_per_block(f) - 1] = -1; // mark the last flash block for copying
	return d;
}

/*
Read a disk block through the flash translation layer.
Go ahead and add or change things here as needed.
*/

int disk_read( struct disk *d, int disk_block, char *data )
{
	printf("disk_read: block %d\n",disk_block);

	// read the correct page from the translation array
	flash_read(d->flash_drive,d->dtf[disk_block],data);

	d->nreads++;
	return 0;
}

/*
Write a disk block through the flash translation layer.
Go ahead and add or change things here as needed.
*/

int disk_write( struct disk *d, int disk_block, const char *data )
{
	printf("disk_write: block %d\n",disk_block);

	// check if overwriting a disk_block
	// mark old flash page stale if so
	if (d->dtf[disk_block] != -1) {
		d->flash_pages[d->dtf[disk_block]] = STALE; // mark it for garbage collection
		d->flash_blocks[d->dtf[disk_block] / flash_npages_per_block(d->flash_drive)] = GARBAGE;
	}

	// look for open flash pages
	while(1) {
		for (int i = 0; i < d->nflash_pages; i++) {
			// need to keep a flash block open for copying, so do not write to the one currently marked for copying
			if(d->flash_blocks[i / flash_npages_per_block(d->flash_drive)] == COPY) {
				printf("Do not write to flash page %d, it is in the copy block\n", i);
				continue;
			}
			if(d->flash_pages[i] == EMPTY) {
				// found empty flash page, write to it
				// update data structures
				d->dtf[disk_block] = i;
				d->flash_pages[i] = disk_block;
				flash_write(d->flash_drive,i,data);
				d->nwrites++;
				return 0;
			}
		}
		garbage_collection(d);
	}
}

/*
Report the total number of operations performed.
You can add more if you like here, but keep the display of reads and writes.
*/

void disk_report( struct disk *d )
{
	printf("\tdisk reads: %d\n",d->nreads);
	printf("\tdisk writes: %d\n",d->nwrites);
}

void garbage_collection( struct disk *d )
{
	printf("Garbage collection started.\n");

	// copy non-stale blocks to the copy block
	// update DS
	// erase the block
	// mark it as copy block

	// find the garbage block to clean
	while(1) {
		int garbage_block = -1;
		for (int i = 0; i < d->nflash_blocks; i++) {
			if (d->flash_blocks[i] == GARBAGE) {
				garbage_block = i;
				break;
			}
		}
		if (garbage_block == -1) {
			printf("No more garbage blocks found.\n");
			return;
		}

		// Find the copy block
		int copy_block = -1;
		for (int i = 0; i < d->nflash_blocks; i++) {
			if (d->flash_blocks[i] == COPY) {
				copy_block = i;
				break;
			}
		}
		if (copy_block == -1) {
			printf("No copy block defined!\n");
			return;
		}

		// found a garbage block, copy pages to copy block
		int pages_per_block = flash_npages_per_block(d->flash_drive); // avoid calling this function a bunch
		int start = garbage_block * pages_per_block;
		int end = start + pages_per_block;

		for (int j = start; j < end; j++) {
			if (d->flash_pages[j] >= 0) {
				int disk_block = d->flash_pages[j];

				// Look for a free page in the copy block
				for (int i = copy_block * pages_per_block; i < (copy_block + 1) * pages_per_block; i++) {
					if (d->flash_pages[i] == EMPTY) {
						char buf[DISK_BLOCK_SIZE];
						flash_read(d->flash_drive, j, buf); // read the data into buffer

						flash_write(d->flash_drive, i, buf); // write the data to the new location

						d->dtf[disk_block] = i;
						d->flash_pages[i] = disk_block;
						d->flash_pages[j] = EMPTY;
						break;
					}
				}
			}
		}

		// do erasing
		flash_erase(d->flash_drive, garbage_block);
		for (int i = start; i < end; i++) {
			// marking the garbage block as emptied
			d->flash_pages[i] = EMPTY;
		}
		d->flash_blocks[copy_block] = USED;
		d->flash_blocks[garbage_block] = COPY; // newly erased block becomes the copy block

		printf("Garbage collection complete. Block %d erased and set as new copy block.\n", garbage_block);
	}
}