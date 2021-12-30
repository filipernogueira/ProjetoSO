#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for (int i = 0; i < BLOCKS; i++) {
                    if (data_block_free(inode->i_data_block[i]) == -1) {
                        return -1;
                    }
                }
                inode->i_size = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}

int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    size_t written_bytes = 0;

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    if (to_write > 0) {
        int full_blocks = (int)inode->i_size / BLOCK_SIZE;
        int blocks_to_write = (int)to_write / BLOCK_SIZE;
        if (to_write % BLOCK_SIZE != 0) {
            blocks_to_write++;
        }

        size_t write_size = BLOCK_SIZE;

        for (int i = full_blocks; i < blocks_to_write + full_blocks; i++) {
            /* Direct Blocks */
            if (i < 10) {
                /* Allocates the block if needed */
                if (inode->i_data_block[i] == -1) {
                    inode->i_data_block[i] = data_block_alloc();
                }

                void *block = data_block_get(inode->i_data_block[i]);
                if (block == NULL) {
                    return -1;
                }

                if (to_write < BLOCK_SIZE) {
                    write_size = to_write;
                }
                /* Perform the actual write */
                memcpy(block + file->of_offset % BLOCK_SIZE, buffer, write_size);

                /* The offset associated with the file handle is
                * incremented accordingly */
                to_write -= write_size;
                file->of_offset += write_size;
                written_bytes += write_size;
            }
            /* Indirect Blocks */
            else {
                int *indirect_block;
                /* Allocates the indirect block, that will store
                * the indexes to the actual blocks */
                if (inode->i_data_block[10] == -1) {
                    inode->i_data_block[10] = data_block_alloc();
                    indirect_block = data_block_get(inode->i_data_block[10]);
                    for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
                        indirect_block[j] = -1;
                    }
                } 
                else {
                    indirect_block = data_block_get(inode->i_data_block[10]);
                    if (indirect_block == NULL) {
                        return -1;
                    }
                }
                /* Allocates the actual block */
                if (indirect_block[i-10] == -1) {
                    indirect_block[i-10] = data_block_alloc();
                }
                void *block = data_block_get(indirect_block[i-10]);

                if (to_write < BLOCK_SIZE) {
                    write_size = to_write;
                }
                /* Perform the actual write */
                memcpy(block + file->of_offset % BLOCK_SIZE, buffer, write_size);

                /* The offset associated with the file handle is
                * incremented accordingly */
                to_write -= write_size;
                file->of_offset += write_size;
                written_bytes += write_size;
            }
        }
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)written_bytes;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    size_t read_bytes = 0;

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        size_t read_size = BLOCK_SIZE;
        int read_blocks = (int)file->of_offset / BLOCK_SIZE;
        int blocks_to_read = (int)to_read / BLOCK_SIZE;
        if (to_read % BLOCK_SIZE != 0) {
            blocks_to_read++;
        }

        for (int i = read_blocks; i < blocks_to_read + read_blocks; i++) {
            /* Direct Blocks */
            if (i < 10) {
                void *block = data_block_get(inode->i_data_block[i]);
                if (block == NULL) {
                    return -1;
                }

                if (to_read < BLOCK_SIZE) {
                    read_size = to_read;
                }
                /* Perform the actual read */
                memcpy(buffer, block + file->of_offset % BLOCK_SIZE, read_size);
                /* The offset associated with the file handle is
                * incremented accordingly */
                to_read -= read_size;
                file->of_offset += read_size;
                read_bytes += read_size;
            }
            /* Indirect Blocks */
            else {
                int *indirect_block = data_block_get(inode->i_data_block[10]);
                void *block = data_block_get(indirect_block[i-10]);

                if (to_read < BLOCK_SIZE) {
                    read_size = to_read;
                }
                /* Perform the actual read */
                memcpy(buffer, block + file->of_offset % BLOCK_SIZE, read_size);
                /* The offset associated with the file handle is
                * incremented accordingly */
                to_read -= read_size;
                file->of_offset += read_size;
                read_bytes += read_size;
            }
        }
    }

    return (ssize_t)read_bytes;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) {
    int source_file, closed, finished = 0;
    FILE *file;
    ssize_t bytes;

    /* Check if the file exists */
    if (tfs_lookup(source_path) == -1) {
        return -1;
    }

    /* Opens the source file */
    source_file = tfs_open(source_path, 0);

    /* Opens or created the OS file */
    file = fopen(dest_path, "w");
    if (file == NULL) {
        return -1;
    }

    /* Copies the file content until reach his end */
    while (finished == 0) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        bytes = tfs_read(source_file, buffer, BUFFER_SIZE);
        if (bytes == -1) {
            return -1;
        }

        fwrite(buffer, sizeof(char), strlen(buffer), file);
        if(bytes < BUFFER_SIZE) {
            finished = 1;
        }
    }

    /* Closes both files */
    closed = tfs_close(source_file);
    if (closed == -1) {
        return -1;
    }
    fclose(file);

    return 0;
}
