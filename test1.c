//
// Created by jian on 4/14/21.
//

#include <stdio.h>
#include "homework.c"

int main() {

    fs_init(NULL);
    //fs_getattr("/dir3/subdir/file.8k-", NULL);
    struct fs_dirent entries[128];
    struct fuse_file_info fi;
    off_t offset = 0;

    fs_readdir("/dir3", entries, test_filler, offset, &fi);
    char buf[15000];

    //fs_read("/dir3/subdir/file.8k-", buf, 30, 0, &fi);
    struct statvfs sfs;
    //fs_statfs("/dir3/subdir/file.8k-", &sfs);
    //int res = fs_rename("dir3/subdir/file.8k-", "dir3/subdir/newFile.8k-");
    mode_t mode = 0777;
    struct fuse_context ctx = { .uid = 500, .gid = 500};

  //  printf("%d", mode);
   // int res = fs_chmod("/dir3/subdir/file.4k-", mode);
    //fs_create("/dir3/subdir/new_Created_File.4k-", mode, &fi);
    //fs_mkdir("/dir2/twenty-seven-byte-file-name", mode);

    //fs_rmdir("/dir2/twenty-seven-byte-file-name");
    //fs_unlink("/dir3/subdir/file.4k-");


}