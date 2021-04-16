/*
 * file:        testing.c
 * description: libcheck test skeleton for file system project
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

struct {
    char *path;
    uint16_t uid;
    uint16_t gid;
    uint32_t mode;
    int32_t  size;
    uint32_t ctime;
    uint32_t mtime;
} table_2[] = {
        {"/", 0, 0, 040777, 4096, 1565283152, 1565283167},
        {"/file.1k", 500, 500, 0100666, 1000, 1565283152, 1565283152},
        {"/file.10", 500, 500, 0100666, 10, 1565283152, 1565283167},
        {"/dir-with-long-name", 0, 0, 040777, 4096, 1565283152, 1565283167},
        {"/dir-with-long-name/file.12k+", 0, 500, 0100666, 12289, 1565283152, 1565283167},
        {"/dir2", 500, 500, 040777, 8192, 1565283152, 1565283167},
        {"/dir2/twenty-seven-byte-file-name", 500, 500, 0100666, 1000, 1565283152, 1565283167},
        {"/dir2/file.4k+", 500, 500, 0100777, 4098, 1565283152, 1565283167},
        {"/dir3", 0, 500, 040777, 4096, 1565283152, 1565283167},
        {"/dir3/subdir", 0, 500, 040777, 4096, 1565283152, 1565283167},
        {"/dir3/subdir/file.4k-", 500, 500, 0100666, 4095, 1565283152, 1565283167},
        {"/dir3/subdir/file.8k-", 500, 500, 0100666, 8190, 1565283152, 1565283167},
        {"/dir3/subdir/file.12k", 500, 500, 0100666, 12288, 1565283152, 1565283167},
        {"/dir3/file.12k-", 0, 500, 0100777, 12287, 1565283152, 1565283167},
        {"/file.8k+", 500, 500, 0100666, 8195, 1565283152, 1565283167}
};
struct {
    char *path;
    int len;
    unsigned cksum; /* UNSIGNED. TESTS WILL FAIL IF IT'S NOT */

} table_1[] = {
        {"/", 120, 1234567},
        {"/another/file", 17, 4567890},
        {NULL}
};
/* change test name and make it do something useful */
START_TEST(a_test)
{
    ck_assert_int_eq(1, 1);

}
END_TEST

START_TEST(fs_getattr_test)
{

    ck_assert_int_eq(1, 1);
    struct stat sb;
    for (int i = 0; i < 15; i++) {
        fs_ops.getattr(table_2[i].path, &sb);
        printf("%d, %d\n", (int)sb.st_mode, table_2[i].mode);
        //ck_assert_int_eq(sb.st_mode, table_2[i].mode);
        ck_assert_uint_eq(sb.st_uid, table_2[i].uid);
        ck_assert_int_eq(sb.st_uid, table_2[i].uid);
        ck_assert_int_eq(sb.st_gid, table_2[i].gid);
        ck_assert_int_eq(sb.st_size, table_2[i].size);
        ck_assert_int_eq(sb.st_ctime, table_2[i].mtime);
        ck_assert_int_eq(sb.st_mtime, table_2[i].ctime);
    }


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
    block_init("test.img");
    fs_ops.init(NULL);


    
    Suite *s = suite_create("fs5600");
    TCase *tc = tcase_create("read_mostly");

    tcase_add_test(tc, a_test); /* see START_TEST above */
    /* add more tests here */
   // TCase *tc1 = tcase_create("fs_getattr");
    tcase_add_test(tc, fs_getattr_test);
    // tcase_add_test(tc, )


    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    int n_failed = srunner_ntests_failed(sr);
    printf("%d tests failed\n", n_failed);
    
    srunner_free(sr);
    return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
