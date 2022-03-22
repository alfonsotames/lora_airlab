/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */

#include <string.h>

#include "file_system.h"
#include "time_system.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(file_system);
#include "time_system.h"


static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";



/* * * * * * * FILESYSTEM INIT AND MOUNT SD CARD * * * * * * * * * * */

 void init_and_mount_sdcard() {
 
	mp.type = FS_FATFS;
	mp.fs_data = &fat_fs;
        
    do {
            const char *disk_pdrv = "SD";
            uint64_t memory_size_mb;
            uint32_t block_count;
            uint32_t block_size;

            if (disk_access_init(disk_pdrv) != 0) {
                    LOG_ERR("Storage init ERROR!");
                    break;
            }

            if (disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
                    LOG_ERR("Unable to get sector count");
                    break;
            }
            LOG_INF("Block count %u", block_count);

            if (disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
                    LOG_ERR("Unable to get sector size");
                    break;
            }
            LOG_INF("Sector size %u", block_size);

            memory_size_mb = (uint64_t)block_count * block_size;
            LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
    } while (0);
    
    
    // Mount Filesystem 
    
    mp.mnt_point = disk_mount_pt;
    int res = fs_mount(&mp);

    if (res == FR_OK) {
            LOG_INF("Disk mounted.");
            lsdir(disk_mount_pt);
    } else {
            LOG_ERR("Error mounting disk.");
    } 
    
    

    
    
                    
          
}

/* * * * * * * * Save data to SD CARD * * * * * * * * * */
 void save_data(char* data, char* type) {
     
    LOG_INF("Saving data...");
    
    

    
    
    char fname[40];
    
    snprintfcb(fname, 40, "/SD:/%s.%s", get_date(), type);
 

        struct fs_file_t file;
        fs_file_t_init(&file);
        LOG_INF("Attemting to write to file: %s",fname);
        int rc = fs_open(&file, fname, FS_O_CREATE | FS_O_APPEND | FS_O_RDWR);
        if (rc < 0) {
                LOG_ERR("FAIL: open file to write: %d", rc);
        }
        rc = fs_write(&file, data, strlen(data));

        if (rc < 0) {
                LOG_ERR("FAIL: cannot write: %d\n", rc);
                k_msleep(500);
        } else {

            LOG_INF(" **** fs_write wrote %d bytes ****",rc);

        }
        fs_close(&file);

 
 }

/* * * * * * * * List Dir Contents * * * * * * * * * */

 int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", path, res);
		return res;
	}

	LOG_INF("\nListing dir %s ...", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s", entry.name);
		} else {
			LOG_INF("[FILE] %s (size = %zu)",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}