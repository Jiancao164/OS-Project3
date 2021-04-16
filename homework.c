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
FILE *fp = NULL;

void* fs_init(struct fuse_conn_info *conn)
{
    /* your code here */
    fp = NULL;
    fp = fopen("test.img", "r");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return NULL;
    }
    printf("test case___");
    fread(&super_block, sizeof(struct fs_super), 1, fp);
    fread(bit_map, sizeof(bit_map), 1, fp);
    fread(&root_inode, sizeof(struct fs_inode), 1, fp);

    fclose(fp);
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


    fp = fopen("test.img", "r");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
    }

    struct fs_inode current_inode = root_inode;
    uint32_t inum = 2;

    for (int i = 0; i < local_pathc; i++) {
        fseek(fp, inum * 4096, SEEK_SET);
        fread(&current_inode, sizeof(struct fs_inode), 1, fp);

        if (current_inode.mode & S_IFDIR) {
            struct fs_dirent entries[128];
            fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);

            unsigned int res = fread(&entries, sizeof(struct fs_inode), 1, fp);

            printf("move return is %ul\n", res);

            printf("last%d\n", current_inode.ptrs[0]);
            printf("mode is %s\n", entries[0].name);
            printf("mode is %d\n", entries[0].valid);
            for (int j = 0; j < 128; j++) {
                if (entries[j].valid) {
                    if (strcmp(entries[j].name, local_pathv[i]) == 0) {
                        inum = entries[j].inode;
                        break;
                    }
                }

                if (j == 127) {
                    printf("return here");
                    return -ENOENT;
                }
            }
            //printf("return here");
//            printf("address is %d\n", ptrs_on_inode[0]);
//            printf("address is %d\n", current_inode.ptrs[1]);
//            printf("address is %d\n", current_inode.ptrs[2]);
//            printf("address is %d\n", current_inode.ptrs[3]);
//            printf("address is %d\n", current_inode.ptrs[4]);
//            printf("address is %d\n", current_inode.ptrs[5]);
//            printf("address is %d\n", current_inode.ptrs[6]);
        } else {
            return -ENOTDIR;
        }
    }

    fclose(fp);
    return inum;
}
int fs_getattr(const char *path, struct stat *sb)
{
    /* your code here */
    //char *argv[];
    //char *argv[10];
    *pathv = NULL;
    parse(path, pathv);
    //pathv = argv;
    printf("path is %s\n", pathv[0]);
    printf("path is %s\n", pathv[1]);
    printf("path is %s\n", pathv[2]);
    printf("has %d fiels\n", pathc);

    uint32_t inum = translate(pathc, pathv);
    printf("nd is %d \n", inum);
    if (inum < 0) return inum;
    fp = NULL;
    fp = fopen("test.img", "r");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
    }

    struct fs_inode attr;
    fseek(fp, inum * 4096, SEEK_SET);
    fread(&attr, sizeof(struct fs_inode), 1, fp);

    printf("size is %d \n", attr.mode);

//    struct fs_dirent *temp = malloc(sizeof (struct fs_dirent));
//
//
    sb = malloc(sizeof(struct stat));
    sb->st_nlink = 1;
    sb->st_uid = attr.uid;
    sb->st_gid = attr.gid;

    sb->st_atime = attr.mtime;
    sb->st_mtime = attr.mtime;
    sb->st_ctime = attr.mtime;
    sb->st_mode = attr.mode;

    sb->st_size = attr.size;
   // sb->
    printf("mode is %d this one\n", sb->st_uid);
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
int test_filler(void *ptr, const char *name,
                const struct stat *st, off_t off)
{
    struct fs_dirent *s = ptr;

    printf("file: %s, mode: %o\n", name, st->st_mode);
    return 0;
}

int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    /* your code here */
    *pathv = NULL;
    parse(path, pathv);
    uint32_t inum = translate(pathc, pathv);

    fp = fopen("test.img", "r");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);
    printf("inum is %d\n", current_inode.size);







    struct fs_dirent entries[128];

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    int res = fread(&ptr, sizeof(struct fs_inode), 1, fp);

   // printf("mode is %d\n", *(struct direct)ptr[0].name);


    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) {
            struct stat *st = malloc(sizeof(struct stat));
            //filler(&ptr[0], entries[i].name, st, 0);
        }
    }

//    filler(ptr, ".", NULL, 0);
//    filler(ptr, "..", NULL, 0);




    return -EOPNOTSUPP;
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

    if (inum > 0) return EEXIST;

    inum = translate(pathc - 1, pathv);
    if (inum < 0) return EEXIST;

    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);


    struct fs_dirent entries[128];;

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    fread(entries, sizeof(struct fs_inode), 1, fp);
    int count_valid = 0;
    long empty_idx = -1;
    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) count_valid++;
        if (!entries[i].valid && empty_idx == -1) empty_idx = i;
    }
    if (count_valid == 128) return -ENOSPC;

    struct fs_dirent new_dir;
    new_dir.valid = 1;
    strncpy(new_dir.name, pathv[pathc - 1], 27);
    new_dir.name[27] = '\0';

    int newInum = -1;

    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            newInum = i;

            bit_set(bit_map, i);
            break;
        }

    }

    if (newInum == -1) return -ENOSPC;

    struct fs_inode new_inode;

    fseek(fp, newInum * 4096, SEEK_SET);
    fread(&new_inode, sizeof(struct  fs_inode), 1, fp);

    new_inode.size = 4096;
    new_inode.mode = mode;
    //struct fuse_context *ctx = fuse_get_context();
    uint16_t uid = 500;
    uint16_t gid = 500;

    new_inode.uid = uid;
    new_inode.gid = gid;

    uint32_t timestamp = time(NULL);
    new_inode.ctime = timestamp;
    new_inode.mtime = timestamp;

    new_dir.inode = newInum;




    fseek(fp, empty_idx * sizeof(struct fs_dirent) + current_inode.ptrs[0] * 4096, SEEK_SET);
    fwrite(&new_dir, sizeof(struct fs_dirent), 1, fp);

    fseek(fp, newInum * 4096, SEEK_SET);
    fwrite(&new_inode, sizeof(struct fs_inode), 1, fp);


    return -EOPNOTSUPP;
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
    printf("__");
    if (inum < 0) return EEXIST;
    printf("__");
    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }
    printf("__");
    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct fs_inode), 1, fp);

    struct fs_dirent entries[128];;

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    fread(entries, sizeof(struct fs_inode), 1, fp);

    int count_valid = 0;
    long empty_idx = -1;
    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) count_valid++;
        if (!entries[i].valid && empty_idx == -1) empty_idx = i;
    }
    if (count_valid == 128) return -ENOSPC;

    struct fs_dirent new_dir;
    new_dir.valid = 1;
    strncpy(new_dir.name, pathv[pathc - 1], 27);
    new_dir.name[27] = '\0';


    int newInum = -1;


    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            newInum = i;
            bit_set(bit_map, i);
            break;
        }

    }
    printf("__sdfds");
    if (newInum == -1) return -ENOSPC;

    struct fs_inode new_inode;

//    fseek(fp, newInum * 4096, SEEK_SET);
//    fread(&new_inode, sizeof(struct  fs_inode), 1, fp);

    new_inode.size = 4096;
    printf("%d, \n", S_IFDIR | mode);
    new_inode.mode = S_IFDIR | mode;

    printf("_______________");
    //struct fuse_context *ctx = fuse_get_context();
    uint16_t uid = 500;
    uint16_t gid = 500;

    new_inode.uid = uid;
    new_inode.gid = gid;

    uint32_t timestamp = time(NULL);
    new_inode.ctime = timestamp;
    new_inode.mtime = timestamp;
    printf("uio%d,\n", newInum);
    new_dir.inode = newInum;



    int new_idx = -1;
    for (int i = 0; i < super_block.disk_size; i++) {
        if (!bit_test(bit_map, i)) {
            new_idx = i;
            bit_set(bit_map, i);
            break;
        }

    }
    printf("%d,,", bit_test(bit_map, new_idx));
    if (new_idx == -1) return -ENOSPC;
    struct fs_dirent new_entries[128];

    new_inode.ptrs[0] = new_idx * 4096;

    fseek(fp, empty_idx * sizeof(struct fs_dirent) + current_inode.ptrs[0] * 4096, SEEK_SET);
    fwrite(&new_dir, sizeof(struct fs_dirent), 1, fp);

//    struct fs_inode temp;
    fseek(fp, newInum * 4096, SEEK_SET);
    fwrite(&new_inode, sizeof(struct fs_inode), 1, fp);

    fseek(fp, new_idx * 4096, SEEK_SET);
    fwrite(new_entries, sizeof(struct fs_dirent) * 128, 1, fp);


    return 0;
}


/* unlink - delete a file
 *  success - return 0
 *  errors - path resolution, ENOENT, EISDIR
 */
int fs_unlink(const char *path)
{
    /* your code here */
    printf("_________");

    *pathv = NULL;
    parse(path, pathv);
    char file_name[28];

    strcpy(file_name, pathv[pathc - 1]);

    int inum = translate(pathc, pathv);
    if (inum < 0) return inum;
    int file_inum = inum;
   // printf("%s is this;;;;", pathv[pathc - 1]);

    inum = translate(pathc - 1, pathv);

    printf("%d\n", inum);

    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct fs_inode), 1, fp);

    struct fs_dirent entries[128];

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    fread(entries, sizeof(struct fs_inode), 1, fp);


    printf("%s \n", entries[0].name);
    printf("%s\n", file_name);

    int idx = -1;
    for (int i = 0; i < 128; i++) {
        if (strcmp(entries[i].name, file_name) == 0) {
            idx = i;
        }
    }

    struct fs_dirent empty_direct;
    fseek(fp, current_inode.ptrs[0] * 4096 + idx * 32, SEEK_SET);
    fwrite(&empty_direct, sizeof(struct fs_dirent), 1, fp);

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    fread(entries, sizeof(struct fs_inode), 1, fp);

    printf("%d, ", entries[0].valid);

    struct fs_inode empty_node;
    fseek(fp, file_inum * 4096, SEEK_SET);
    fwrite(&empty_node, sizeof(struct fs_inode), 1, fp);

    bit_clear(bit_map, file_inum);

    // need to remove block data

    return -EOPNOTSUPP;
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

    printf("%d\n", inum);

    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct fs_inode), 1, fp);

    if (current_inode.mode & 0100000) return ENOTDIR;


    printf("%d,,\n", current_inode.mode);
    struct fs_dirent entries[128];

    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);
    fread(entries, sizeof(struct fs_inode), 1, fp);

    for (int i = 0; i < 128; i++) {
        if (entries[i].valid) return ENOTEMPTY;
    }
    int p_inum = translate(pathc - 1, pathv);

    struct fs_inode parent_inode;

    fseek(fp, p_inum * 4096, SEEK_SET);
    fread(&parent_inode, sizeof(struct fs_inode), 1, fp);


    struct fs_dirent parent_entries[128];

    fseek(fp, parent_inode.ptrs[0] * 4096, SEEK_SET);
    fread(parent_entries, sizeof(struct fs_inode), 1, fp);

    int idx = -1;
    for (int i = 0; i < 128; i++) {
        if (strcmp(parent_entries->name, directory_name) == 0) {
            idx = i;
        }
    }
    struct fs_dirent empty_direct;

    fseek(fp, p_inum * 4096 + idx * 32, SEEK_SET);
    fwrite(&empty_direct, sizeof(struct fs_dirent), 1, fp);

    struct fs_inode empty_node;
    fseek(fp, inum * 4096, SEEK_SET);
    fwrite(&empty_node, sizeof(struct fs_inode), 1, fp);

    bit_clear(bit_map, inum);


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

    printf("__");
    parse(src_path, pathv);
    char *src_pathv[10];
    printf("__");
    int count = 0;
    for (int i = 0; i < 10; i++) {
        printf("__");
        if (pathv[i]) {
            printf("__");
            count++;
            src_pathv[i] = malloc(28);
           strncpy(src_pathv[i], pathv[i], 28);
        }
    }
    int count_dst = 0;
    char *dst_pathv[10];
    parse(dst_path, pathv);
    for (int i = 0; i < 10; i++) {
        printf("__");
        if (pathv[i]) {
            printf("__");
            count_dst++;
            dst_pathv[i] = malloc(28);
            strncpy(dst_pathv[i], pathv[i], 28);
        }
    }
//    //strncpy(*src_pathv, *pathv, 10);
    printf("%d\n", count);

    if (count != count_dst) return -EINVAL;
    if (strcmp(dst_pathv[count - 1], src_pathv[count - 1]) == 0) return -EEXIST;

    for (int i = 0; i < count - 1; i++) {
        if (strcmp(dst_pathv[i], src_pathv[i]) != 0) return -EINVAL;
    }

    uint32_t inum = translate(count - 1, src_pathv);
    fp = NULL;
    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -ENOENT;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);
    printf("inum is %d\n", current_inode.size);

    struct fs_dirent entries[128];
    fseek(fp, current_inode.ptrs[0] * 4096, SEEK_SET);

    unsigned int res = fread(&entries, sizeof(struct fs_inode), 1, fp);
    int idx = -1;
    for (int i = 0; i < 128; i++) {
        if (strcmp(entries[i].name, dst_pathv[count_dst - 1]) == 0) return -EEXIST;
        if (strcmp(entries[i].name, src_pathv[count - 1]) == 0) idx = i;
        //printf("name is%s, %s\n", entries[i].name, src_pathv[count - 1]);
    }
    if (idx == -1) return -ENOENT;
    //entries[idx].name = dst_pathv[count_dst - 1];
    strcpy(entries[idx].name, dst_pathv[count_dst - 1]);
    printf("%s\n", entries[idx].name);
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

    fp = NULL;
    fp = fopen("test.img", "r+");
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -ENOENT;
    }

    struct fs_inode current_inode;

    fseek(fp, inum * 4096, SEEK_SET);
    fread(&current_inode, sizeof(struct  fs_inode), 1, fp);
    printf("inum is %d\n", inum);
    printf("mode is %u", current_inode.mode); // 33206
    current_inode.mode = mode & 0777;
    printf("mode is %u", current_inode.mode); // 33206




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
    uint32_t inum = translate(pathc, pathv);
    if (inum < 0) return (int)inum;

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
  //  printf("%d \n", idx_offset);
 //   printf("%d \n", idx_block);
    printf("%d\n", current_inode.ptrs[idx_block]);
    while (len > 0) {
        fseek(fp, current_inode.ptrs[idx_block++] * 4096 + offset, SEEK_SET);
        size_t read_bytes = idx_offset + len > 4096? 4096 - offset : len;

        count_byte += read_bytes;

        //printf("%d\n", read_bytes);
        int res = fread(buf, read_bytes, 1, fp);
        printf("%d\n", res);
        idx_offset = 0;
        len -= read_bytes;
    }

    printf("address is %s", buf);
    //current_inode.ptrs;





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
    uint32_t inum = translate(pathc, pathv);
    if (inum < 0) return (int)inum;

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
    if (fp == NULL) {
        fprintf(stderr, "failed.");
        return -1;
    }

    fread(&super_block, sizeof(struct fs_super), 1, fp);
    fread(bit_map, sizeof(bit_map), 1, fp);
    fread(&root_inode, sizeof(struct fs_inode), 1, fp);

    st->f_bsize = sizeof(super_block);
    st->f_blocks = super_block.disk_size;

    int blocks_used = 0;
    for (int i = 0; i < sizeof(bit_map) * 8; i++) {
        if(bit_test(bit_map, i) > 0) blocks_used++;
        //if (bit_map[i] != 0) blocks_used++;
    }
    st->f_bfree = blocks_used;
    st->f_bavail = super_block.disk_size - blocks_used;
    struct fs_dirent f;
    st->f_namemax = sizeof(f.name) - 1;
    printf("used is %lu \n", st->f_namemax);


    return -EOPNOTSUPP;
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

