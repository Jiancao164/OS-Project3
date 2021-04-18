/*
 * file:        unittest-2.c
 * description: libcheck test skeleton, part 2
 */

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <zlib.h>
#include <fuse.h>
#include <stdlib.h>
#include <errno.h>

extern struct fuse_operations fs_ops;
extern void block_init(char *file);
/* mockup for fuse_get_context. you can change ctx.uid, ctx.gid in 
 * tests if you want to test setting UIDs in mknod/mkdir
 */
struct fuse_context ctx = { .uid = 500, .gid = 500};
struct fuse_context *fuse_get_context(void)
{
    return &ctx;
}

/* change test name and make it do something useful */
START_TEST(a_test)
{
    ck_assert_int_eq(1, 1);

}
END_TEST

START_TEST(fs_create_test)
    {
        fs_ops.create("/file1.0", 0100777, NULL);
        struct stat st;
        fs_ops.getattr("/file1.0", &st);
        ck_assert_int_eq(st.st_mode, 0100777);

        fs_ops.create("/dir1/file2.0", 0100777, NULL);
        struct stat st1;
        fs_ops.getattr("/dir1/file2.0", &st1);
        ck_assert_int_eq(st1.st_mode, 0100777);

        fs_ops.create("/dir1/dir2/file3.0", 0100777, NULL);
        struct stat st3;
        fs_ops.getattr("/dir1/dir2/file3.0", &st3);
        ck_assert_int_eq(st1.st_mode, 0100777);

        ck_assert_int_eq(fs_ops.create("/dir1/dir3/file4.0", 0100777, NULL), -ENOENT);
        ck_assert_int_eq(fs_ops.create("/file1.0/file4.0", 0100777, NULL), -ENOENT);
        ck_assert_int_eq(fs_ops.create("/file1.0", 0100777, NULL), -EEXIST);
        ck_assert_int_eq(fs_ops.create("/dir1", 0100777, NULL), -EEXIST);


        fs_ops.create("/too_long_name_to_long_name_too_long_name_to_"
                      "long_name_too_long_name_to_long_name_too_long_name_to_long_name_"
                      "too_long_name_to_long_name_too_long_name_to_long_name", 0100777, NULL);
        struct stat st4;
        fs_ops.getattr("/too_long_name_to_long_name_", &st4);
        ck_assert_int_eq(st4.st_mode, 0100777);


    }
END_TEST

START_TEST(fs_mkdir_test)
    {
        fs_ops.mkdir("/dir1", 0777);
        struct stat st;
        fs_ops.getattr("/dir1", &st);
        ck_assert_int_eq(st.st_mode, 040777);

        fs_ops.mkdir("/dir1/dir2", 0777);
        struct stat st1;
        fs_ops.getattr("/dir1/dir2", &st1);
        ck_assert_int_eq(st1.st_mode, 040777);

        ck_assert_int_eq(fs_ops.mkdir("/dir1/dir9/dir4", 0777), -ENOENT);
        ck_assert_int_eq(fs_ops.mkdir("/file1.0/dir4", 0777), -ENOENT);

        fs_ops.create("/file9.0", 0100777, NULL);
        ck_assert_int_eq(fs_ops.mkdir("/file9.0", 0777), -EEXIST);
        ck_assert_int_eq(fs_ops.mkdir("/dir1", 0777), -EEXIST);


        fs_ops.mkdir("/dir_too_long_name_to_long_name_too_long_name_to_"
                      "long_name_too_long_name_to_long_name_too_long_name_to_long_name_"
                      "too_long_name_to_long_name_too_long_name_to_long_name", 0777);
        struct stat st4;
        fs_ops.getattr("/dir_too_long_name_to_long_n", &st4);
        ck_assert_int_eq(st.st_mode, 040777);

    }
END_TEST

START_TEST(fs_rmdir_test)
    {
        fs_ops.rmdir("/dir2");

        struct stat st;
        ck_assert_int_lt(fs_ops.getattr("/dir2", &st), 0);
        ck_assert_int_eq(fs_ops.rmdir("/dir1/dir3/file4.0"), -ENOENT);
        ck_assert_int_eq(fs_ops.rmdir("/file1.0/file4.0"), -ENOTDIR);
        ck_assert_int_eq(fs_ops.rmdir("/dir19.0"), -ENOENT);
        ck_assert_int_eq(fs_ops.rmdir("/dir1/file2.0"), -ENOTDIR);
        ck_assert_int_eq(fs_ops.rmdir("/dir1"), -ENOTEMPTY);

    }
END_TEST

START_TEST(fs_unlink_test)
    {
        fs_ops.unlink("/file1.0");
        struct stat st;
        ck_assert_int_lt(fs_ops.getattr("/file1.0", &st), 0);
        ck_assert_int_eq(fs_ops.unlink("/dir1/dir3/file4.0"), -ENOENT);
        ck_assert_int_eq(fs_ops.unlink("/file1.0/file4.0"), -ENOENT);
        ck_assert_int_eq(fs_ops.unlink("/file19.0"), -ENOENT);
        ck_assert_int_eq(fs_ops.unlink("/dir1"), -EISDIR);



    }
END_TEST

START_TEST(fs_write_test)
    {
        char *ptr, *buf = malloc(4010); // allocate a bit extra
        int i;
        for (i=0, ptr = buf; ptr < buf+4000; i++) {
            ptr += sprintf(ptr, "%d ", i);
        }
        fs_ops.create("/file7.4k", 0100777, NULL);
        int c = fs_ops.write("/file7.4k", buf, 4096, 0, NULL); // 4000 bytes, offset=0
        char *buf1 = malloc(4096);
        fs_ops.read("/file7.4k", buf1, 4096, 0, NULL);

    }
END_TEST

START_TEST(fs_truncate_test)
    {
        char *buf1 = malloc(4096);
        fs_ops.read("/file7.4k", buf1, 4096, 0, NULL);
        fs_ops.truncate("/file7.4k", 0);
        char *buf2 = malloc(4096);
        fs_ops.read("/file7.4k", buf2, 4096, 0, NULL);



    }
END_TEST

START_TEST(fs_utime_test)
    {
        struct utimbuf *uti = malloc(sizeof(struct utimbuf));
        uti->actime = 200;
        uti->modtime = 200;
        fs_ops.utime("/file7.4k", uti);

        struct stat *st = malloc(sizeof(struct stat));
        fs_ops.getattr("/file7.4k", st);

        ck_assert_int_eq(st->st_mtim.tv_sec, 200);
        ck_assert_int_eq(st->st_atim.tv_sec, 200);
    }
END_TEST


/* this is an example of a callback function for readdir
 */
int empty_filler(void *ptr, const char *name, const struct stat *stbuf,
                 off_t off)
{
    /* FUSE passes you the entry name and a pointer to a 'struct stat' 
     * with the attributes. Ignore the 'ptr' and 'off' arguments 
     * 
     */
    return 0;
}

/* note that your tests will call:
 *  fs_ops.getattr(path, struct stat *sb)
 *  fs_ops.readdir(path, NULL, filler_function, 0, NULL)
 *  fs_ops.read(path, buf, len, offset, NULL);
 *  fs_ops.statfs(path, struct statvfs *sv);
 */



int main(int argc, char **argv)
{
    block_init("test2.img");
    fs_ops.init(NULL);

    Suite *s = suite_create("fs5600");
    TCase *tc = tcase_create("write_mostly");

    tcase_add_test(tc, a_test); /* see START_TEST above */
    /* add more tests here */
    tcase_add_test(tc, fs_mkdir_test);
    tcase_add_test(tc, fs_create_test);
    tcase_add_test(tc, fs_rmdir_test);
    tcase_add_test(tc, fs_unlink_test);
    tcase_add_test(tc, fs_write_test);
    tcase_add_test(tc, fs_truncate_test);
    tcase_add_test(tc, fs_utime_test);

    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    int n_failed = srunner_ntests_failed(sr);
    printf("%d tests failed\n", n_failed);
    
    srunner_free(sr);
    return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

