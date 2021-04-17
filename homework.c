/*
 * file: homework.c
 * description: skeleton file for CS 5600 system
 *
 * CS 5600, Computer Systems, Northeastern
 * Created by: Peter Desnoyers, November 2019
 */

#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "fs5600.h"

/* if you don't understand why you can't use these system calls here, 
 * you need to read the assignment description another time
 */
#define stat(a,b) error do not use stat()
#define open(a,b) error do not use open()
#define read(a,b,c) error do not use read()
#define write(a,b,c) error do not use write()

/* disk access. All access is in terms of 4KB blocks; read and
 * write functions return 0 (success) or -EIO.
 */
extern int block_read(void *buf, int lba, int nblks);
extern int block_write(void *buf, int lba, int nblks);

/* bitmap functions
 */
void bit_set(unsigned char *map, int i)
{
    map[i/8] |= (1 << (i%8));

}
void bit_clear(unsigned char *map, int i)
{
    map[i/8] &= ~(1 << (i%8));
}
int bit_test(unsigned char *map, int i)
{
    return map[i/8] & (1 << (i%8));
}


/* init - this is called once by the FUSE framework at startup. Ignore
 * the 'conn' argument.
 * recommended actions:
 *   - read superblock
 *   - allocate memory, read bitmaps and inodes
 */

struct fs_super super_block;
unsigned char bit_map[4 * 1024];
struct fs_inode root_inode;
static FILE *fp;

void* fs_init(struct fuse_conn_info *conn)
{
    /* your code here */
    block_read(&super_block, 0, 1);
    block_read(bit_map, 1, 1);
    block_read(&root_inode, 2, 1);
    return NULL;
}

/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path doesn't exist.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */

/* note on splitting the 'path' variable:
 * the value passed in by the FUSE framework is declared as 'const',
 * which means you can't modify it. The standard mechanisms for
 * splitting strings in C (strtok, strsep) modify the string in place,
 * so you have to copy the string and then free the copy when you're
 * done. One way of doing this:
 *
 *    char *_path = strdup(path);
 *    int inum = translate(_path);
 *    free(_path);
 */



#define MAX_PATH_LEN 10
#define MAX_NAME_LEN 27
int pathc;
char *pathv[10];

int parse(const char *path, char **argv)
{
    char *_path = strdup(path);
    int i;

    for (i = 0; i < MAX_PATH_LEN; i++) {
        if ((argv[i] = strtok(_path, "/")) == NULL)
            break;
        if (strlen(argv[i]) > MAX_NAME_LEN)
            argv[i][MAX_NAME_LEN] = 0; // truncate to 27 characters
        _path = NULL;
    }

    pathc = i;
    free(_path);
    return i;
}


int translate(int local_pathc, char **local_pathv) {

    struct fs_inode current_inode = root_inode;
    int inum = 2;

    for (int i = 0; i < local_pathc; i++) {
        block_read(&current_inode, inum, 1);

        if (current_inode.mode & S_IFDIR) {
            struct fs_dirent entries[128];
            block_read(entries, current_inode.ptrs[0], 1);

            for (int j = 0; j < 128; j++) {
                if (entries[j].valid) {
                    if (strcmp(entries[j].name, local_pathv[i]) == 0) {
                        inum = entries[j].inode;
                        break;
                    }
                }

                if (j == 127) {
                    return -ENOENT;
                }
            }
        } else {
            return -ENOTDIR;
        }
    }


    return inum;
}

/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - for several fields in 'struct stat' there is no corresponding
 *  information in our file system:
 *    st_nlink - always set it to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * success - return 0
 * errors - path translation, ENOENT
 * hint - factor out inode-to-struct stat conversion - you'll use it
 *        again in readdir
 */
int fs_getattr(const char *path, struct stat *sb)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);

    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    struct fs_inode attr;
    block_read(&attr, inum, 1);

    sb->st_nlink = 1;
    sb->st_uid = attr.uid;
    sb->st_gid = attr.gid;
    sb->st_atime = attr.mtime;
    sb->st_mtime = attr.mtime;
    sb->st_ctime = attr.ctime;
    sb->st_mode = attr.mode;
    sb->st_size = attr.size;

    return 0;
}

/* readdir - get directory contents.
 *
 * call the 'filler' function once for each valid entry in the
 * directory, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a pointer to a struct stat
 * success - return 0
 * errors - path resolution, ENOTDIR, ENOENT
 *
 * hint - check the testing instructions if you don't understand how
 *        to call the filler function
 */

int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    struct fs_dirent entries[128];
    block_read(entries, current_inode.ptrs[0], 1);

    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) {
            filler(ptr, entries[i].name, NULL, i);
        }
    }

    return 0;
}

/* create - create a new file with specified permissions
 *
 * success - return 0
 * errors - path resolution, EEXIST
 *          in particular, for create("/a/b/c") to succeed,
 *          "/a/b" must exist, and "/a/b/c" must not.
 *
 * Note that 'mode' will already have the S_IFREG bit set, so you can
 * just use it directly. Ignore the third parameter.
 *
 * If a file or directory of this name already exists, return -EEXIST.
 * If there are already 128 entries in the directory (i.e. it's filled an
 * entire block), you are free to return -ENOSPC instead of expanding it.
 */
int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum > 0) return inum;
    printf("now is %d", inum);
    inum = translate(pathc - 1, pathv);
    if (inum < 0) return inum;

    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    if (current_inode.mode & 0100000) return -ENOENT;

    struct fs_dirent *entries = malloc(4096);
    block_read(entries, current_inode.ptrs[0], 1);

    int count_valid = 0;
    long empty_idx = -1;
    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) count_valid++;
        if (!entries[i].valid && empty_idx == -1) empty_idx = i;
    }
    if (count_valid == 128) return -ENOSPC;

    entries[empty_idx].valid = 1;
    strncpy(entries[empty_idx].name, pathv[pathc - 1], 27);
    int newInum = -1;

    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            newInum = i;
            bit_set(bit_map, i);
            break;
        }
    }

    if (newInum == -1) return -ENOSPC;
    struct fs_inode *new_inode = malloc(sizeof(struct fs_inode));

    new_inode->size = 4096;
    new_inode->mode = mode;
    uint16_t uid = 500;
    uint16_t gid = 500;
    new_inode->uid = uid;
    new_inode->gid = gid;
    uint32_t timestamp = time(NULL);
    new_inode->ctime = timestamp;
    new_inode->mtime = timestamp;

    for (int i = 0; i < 400; i++) {
        if (bit_test(bit_map, i) == 0) {
            bit_set(bit_map, i);
            new_inode->ptrs[0] = i;
            break;
        }
    }

    entries[empty_idx].inode = (uint32_t)newInum;
    block_write(entries, current_inode.ptrs[0], 1);
    block_write(bit_map, 1, 1);
    block_write(new_inode, newInum, 1);
    return 0;
}

/* mkdir - create a directory with the given mode.
 *
 * WARNING: unlike fs_create, @mode only has the permission bits. You
 * have to OR it with S_IFDIR before setting the inode 'mode' field.
 *
 * success - return 0printf("__sdfds");
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create. 
 */ 
int fs_mkdir(const char *path, mode_t mode)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum > 0) return EEXIST;
    inum = translate(pathc - 1, pathv);
    if (inum < 0) return EEXIST;

    struct fs_inode current_inode;
    if (current_inode.mode & 0100000) return -ENOENT;
    block_read(&current_inode, inum, 1);
    struct fs_dirent entries[128];;
    block_read(entries, current_inode.ptrs[0], 1);

    int count_valid = 0;
    long empty_idx = -1;
    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) count_valid++;
        if (!entries[i].valid && empty_idx == -1) empty_idx = i;
    }
    if (count_valid == 128) return -ENOSPC;

    entries[empty_idx].valid = 1;
    strncpy(entries[empty_idx].name, pathv[pathc - 1], 28);
    int newInum = -1;
    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            newInum = i;
            bit_set(bit_map, i);
            break;
        }
    }
    block_write(bit_map, 1, 1);
    if (newInum == -1) return -ENOSPC;

    struct fs_inode *new_inode = malloc(4096);

    new_inode->size = 4096;
    new_inode->mode = 040000 | mode;
    //struct fuse_context *ctx = fuse_get_context();
    uint16_t uid = 500;
    uint16_t gid = 500;
    new_inode->uid = uid;
    new_inode->gid = gid;
    uint32_t timestamp = time(NULL);
    new_inode->ctime = timestamp;
    new_inode->mtime = timestamp;
    entries[empty_idx].inode = newInum;

    int new_idx = -1;
    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            new_idx = i;
            bit_set(bit_map, i);
            break;
        }
    }

    if (new_idx == -1) return -ENOSPC;
    new_inode->ptrs[0] = new_idx;
    struct fs_dirent *new_entries = calloc(sizeof(struct fs_dirent) , 128);

    block_write(entries, current_inode.ptrs[0], 1);
    block_write(new_inode, newInum, 1);
    bit_set(bit_map, new_idx);
    block_write(bit_map, 1, 1);
    block_write(new_entries, new_idx, 1);
    return 0;
}


/* unlink - delete a file
 *  success - return 0
 *  errors - path resolution, ENOENT, EISDIR
 */
int fs_unlink(const char *path)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    char file_name[28];
    strcpy(file_name, pathv[pathc - 1]);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;
    int file_inum = inum;
    inum = translate(pathc - 1, pathv);

    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    if (current_inode.mode & 0100000) return -ENOENT;

    struct fs_inode *file_node = malloc(4096);
    block_read(file_node, file_inum, 1);

    for (int i = 0; i < FS_BLOCK_SIZE/4 - 5; i++) {
        if (file_node->ptrs[i] != 0) {
            block_write("", file_node->ptrs[i], 1);
            bit_clear(bit_map, file_node->ptrs[i]);
        }
    }



    struct fs_dirent entries[128];
    block_read(entries, current_inode.ptrs[0], 1);

    for (int i = 0; i < 128; i++) {
        if (strcmp(entries[i].name, file_name) == 0) {
            strcpy(entries[i].name, "");
            entries[i].valid = 0;
            entries[i].inode = 0;
        }
    }

    block_write(entries, current_inode.ptrs[0], 1);
    struct fs_inode *empty_node = malloc(4096);
    block_write(empty_node, file_inum, 1);
    bit_clear(bit_map, file_inum);
    block_write(bit_map, 1, 1);
    return 0;
}

/* rmdir - remove a directory
 *  success - return 0
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 */
int fs_rmdir(const char *path)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);

    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;
    char directory_name[28];
    strcpy(directory_name, pathv[pathc - 1]);
    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    if (current_inode.mode & 0100000) return ENOTDIR;
    struct fs_dirent entries[128];

    block_read(entries, current_inode.ptrs[0], 1);
    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) return ENOTEMPTY;
    }
    int p_inum = translate(pathc - 1, pathv);
    struct fs_inode parent_inode;
    block_read(&parent_inode, p_inum, 1);
    struct fs_dirent *parent_entries = calloc(128, sizeof(struct fs_dirent));
    block_read(parent_entries, parent_inode.ptrs[0], 1);

    for (int i = 0; i < 128; i++) {
        if (parent_entries[i].valid) {
            if (strcmp(parent_entries[i].name, directory_name) == 0) {
                strcpy(parent_entries[i].name, "");
                parent_entries[i].valid = 0;
                parent_entries[i].inode = 0;
            }
        }
    }

    block_write(parent_entries, parent_inode.ptrs[0], 1);
    struct fs_inode *empty_node = malloc(sizeof(struct fs_inode));
    block_write(empty_node, inum, 1);
    bit_clear(bit_map, inum);
    bit_clear(bit_map, current_inode.ptrs[0]);
    block_write(bit_map, 1, 1);

    return 0;
}

/* rename - rename a file or directory
 * success - return 0
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
int fs_rename(const char *src_path, const char *dst_path)
{
    /* your code here */
    *pathv = NULL;

    parse(src_path, pathv);
    char *src_pathv[10];
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (pathv[i]) {
            count++;
            src_pathv[i] = malloc(28);
           strncpy(src_pathv[i], pathv[i], 28);
        }
    }
    int count_dst = 0;
    char *dst_pathv[10];
    parse(dst_path, pathv);
    for (int i = 0; i < 10; i++) {
        if (pathv[i]) {
            count_dst++;
            dst_pathv[i] = malloc(28);
            strncpy(dst_pathv[i], pathv[i], 28);
        }
    }

    if (count != count_dst) return -EINVAL;
    if (strcmp(dst_pathv[count - 1], src_pathv[count - 1]) == 0) return -EEXIST;

    for (int i = 0; i < count - 1; i++) {
        if (strcmp(dst_pathv[i], src_pathv[i]) != 0) return -EINVAL;
    }

    int inum = translate(count - 1, src_pathv);

    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    struct fs_dirent entries[128];
    block_read(entries, current_inode.ptrs[0], 1);

    int idx = -1;
    for (int i = 0; i < 128; i++) {
        if (strcmp(entries[i].name, dst_pathv[count_dst - 1]) == 0) return -EEXIST;
        if (strcmp(entries[i].name, src_pathv[count - 1]) == 0) idx = i;
    }
    if (idx == -1) return -ENOENT;
    strcpy(entries[idx].name, dst_pathv[count_dst - 1]);
    block_write(entries, current_inode.ptrs[0], 1);

    return 0;
}

/* chmod - change file permissions
 * utime - change access and modification times
 *         (for definition of 'struct utimebuf', see 'man utime')
 *
 * success - return 0
 * Errors - path resolution, ENOENT.
 */
int fs_chmod(const char *path, mode_t mode)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    struct fs_inode current_inode;
    block_read(&current_inode, inum, 1);
    current_inode.mode = current_inode.mode & 49152;
    current_inode.mode = mode | current_inode.mode;
    block_write(&current_inode, inum, 1);

    return 0;
}

int fs_utime(const char *path, struct utimbuf *ut)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    fp = NULL;
    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -ENOENT;
    }



    struct fs_inode current_inode;
    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);

    current_inode.mtime = ut->modtime;
    current_inode.ctime = ut->actime;

    fseek(fp, inum * 4096, SEEK_SET);
    fwrite(&current_inode, sizeof(current_inode), 1, fp);

    return 0;
}

/* truncate - truncate file to exactly 'len' bytes
 * success - return 0
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
int fs_truncate(const char *path, off_t len)
{
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */
    if (len != 0)
	return -EINVAL;		/* invalid argument */

    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;


    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);

    if (current_inode.mode & 040000) return -EISDIR;



    for (int i = 0; i < 1019; i++) {
        if (current_inode.ptrs[i]) {
            uint32_t add = current_inode.ptrs[i];
            fseek(fp, add, SEEK_SET);
            fwrite("", 4096, 1, fp);
        }
    }


    return 0;
}


/* read - read data from an open file.
 * success: should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return #bytes from offset to end
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
int fs_read(const char *path, char *buf, size_t len, off_t offset,
	    struct fuse_file_info *fi)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    fp = fopen("test.img", "r");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);


    if ((FS_BLOCK_SIZE/4 - 5) * 4096 <= offset) return 0;
    if (offset + len > (FS_BLOCK_SIZE/4 - 5) * 4096) {
        len = (FS_BLOCK_SIZE/4 - 5) * 4096 - offset;
    }
    size_t count_byte = 0;

    long idx_block = offset / 4096;
    long idx_offset = offset % 4096;

    while (len > 0) {
        fseek(fp, current_inode.ptrs[idx_block++] * 4096 + idx_offset, SEEK_SET);
        size_t read_bytes = idx_offset + len > 4096? 4096 - idx_offset : len;
        fread(&buf[count_byte + offset], read_bytes, 1, fp);
        count_byte += read_bytes;
        idx_offset = 0;
        len -= read_bytes;
    }

    return (int)count_byte;
}

/* write - write data to a file
 * success - return number of bytes written. (this will be the same as
 *           the number requested, or else it's an error)
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 *  (POSIX semantics support the creation of files with "holes" in them, 
 *   but we don't)
 */
int fs_write(const char *path, const char *buf, size_t len,
	     off_t offset, struct fuse_file_info *fi)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;

    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);

    if (current_inode.mode & 040000) return -EISDIR;

    if ((FS_BLOCK_SIZE/4 - 5) * 4096 > offset) return -EINVAL;


    if (offset + len > (FS_BLOCK_SIZE/4 - 5) * 4096) {
        len = (FS_BLOCK_SIZE/4 - 5) * 4096 - offset;
    }
    size_t count_byte = 0;

    long idx_block = offset / 4096;
    long idx_offset = offset % 4096;
    //  printf("%d \n", idx_offset);
    //   printf("%d \n", idx_block);



    printf("%d\n", current_inode.ptrs[idx_block]);
    size_t idx = 0;
    while (len > 0) {
        fseek(fp, current_inode.ptrs[idx_block++] * 4096 + offset, SEEK_SET);
        size_t read_bytes = idx_offset + len > 4096? 4096 - offset : len;

        count_byte += read_bytes;

        //printf("%d\n", read_bytes);

        fwrite(&buf[idx], read_bytes, 1, fp);
        idx += read_bytes;

        idx_offset = 0;
        len -= read_bytes;
    }

    return count_byte;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. Needs to work.
 */
int fs_statfs(const char *path, struct statvfs *st)
{
    /* needs to return the following fields (set others to zero):
     *   f_bsize = BLOCK_SIZE
     *   f_blocks = total image - (superblock + block map)
     *   f_bfree = f_blocks - blocks used
     *   f_bavail = f_bfree
     *   f_namemax = <whatever your max namelength is>
     *
     * it's OK to calculate this dynamically on the rare occasions
     * when this function is called.
     */
    /* your code here */

    fp = NULL;
    fp = fopen("test.img", "r");
//    if (fp == NULL) {
//        fprintf(stderr, "failed.");
//        return -1;
//    }

    block_read(&super_block, 0, 1);
    block_read(bit_map, 1, 1);
    block_read(&root_inode, 2, 1);

    st->f_bsize = sizeof(super_block);
    st->f_blocks = super_block.disk_size;

    int blocks_used = 0;
    for (int i = 0; i < sizeof(bit_map) * 8; i++) {
        if(bit_test(bit_map, i) > 0) blocks_used++;
    }
    st->f_bfree = super_block.disk_size - blocks_used;
    st->f_bavail = super_block.disk_size - blocks_used;
    struct fs_dirent f;
    st->f_namemax = sizeof(f.name) - 1;

    return 0;
}

/* operations vector. Please don't rename it, or else you'll break things
 */
struct fuse_operations fs_ops = {
    .init = fs_init,            /* read-mostly operations */
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .rename = fs_rename,
    .chmod = fs_chmod,
    .read = fs_read,
    .statfs = fs_statfs,

    .create = fs_create,        /* write operations */
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .utime = fs_utime,
    .truncate = fs_truncate,
    .write = fs_write,
};

