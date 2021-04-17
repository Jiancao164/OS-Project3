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

    }
END_TEST

START_TEST(fs_rmdir_test)
    {
        fs_ops.rmdir("/dir2");

        struct stat st;
        ck_assert_int_lt(fs_ops.getattr("/dir2", &st), 0);

    }
END_TEST

START_TEST(fs_unlink_test)
    {
        fs_ops.unlink("/file1.0");
        struct stat st;
        ck_assert_int_lt(fs_ops.getattr("/file1.0", &st), 0);

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
    //tcase_add_test(tc, fs_mkdir_test);
    tcase_add_test(tc, fs_create_test);

//    tcase_add_test(tc, fs_rmdir_test);

   // tcase_add_test(tc, fs_unlink_test);

    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    int n_failed = srunner_ntests_failed(sr);
    printf("%d tests failed\n", n_failed);
    
    srunner_free(sr);
    return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

