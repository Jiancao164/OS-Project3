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

    struct stat *sb = malloc(sizeof(const struct stat));
    for (int i = 0; i < 15; i++) {
        fs_ops.getattr(table_2[i].path, sb);

        ck_assert_int_eq(sb->st_mode, table_2[i].mode);
        ck_assert_uint_eq(sb->st_uid, table_2[i].uid);
        ck_assert_int_eq(sb->st_uid, table_2[i].uid);
        ck_assert_int_eq(sb->st_gid, table_2[i].gid);
        ck_assert_int_eq(sb->st_size, table_2[i].size);
        ck_assert_int_eq(sb->st_ctime, table_2[i].ctime);
        ck_assert_int_eq(sb->st_mtime, table_2[i].mtime);
    }

    int code = fs_ops.getattr("not-a-file", sb);
    ck_assert_int_eq(code, -ENOENT);

    int code2 = fs_ops.getattr("/file.1k/file.0", sb);
    ck_assert_int_eq(code2, -ENOTDIR);

    int code3 = fs_ops.getattr("/not-a-dir/file.0", sb);
    ck_assert_int_eq(code3, -ENOENT);

    int code4 = fs_ops.getattr("/dir2/not-a-file", sb);
    ck_assert_int_eq(code4, -ENOENT);

}
END_TEST

START_TEST(fs_statfs_test)
    {
        ck_assert_int_eq(1, 1);
        struct statvfs *sf = calloc(1, sizeof(statvfs));
        fs_ops.statfs("/", sf);

        ck_assert_int_eq(sf->f_bsize, 4096);
        ck_assert_int_eq(sf->f_blocks, 400);
        ck_assert_int_eq(sf->f_bfree, 355);
        ck_assert_int_eq(sf->f_namemax, 27);


    }
END_TEST

struct dir_name {
    char *name;
    int seen;
} dir1_table[] = {
        {"dir2", 0},
        {"dir3", 0},
        {"dir-with-long-name", 0},
        {"file.10", 0},
        {"file.1k", 0},
        {"file.8k+", 0}
}, dir2_table[] = {
        {"twenty-seven-byte-file-name", 0},
        {"file.4k+", 0}
}, dir3_table[] = {
        {"subdir", 0},
        {"file.12k-", 0}
}, dir4_table[] = {
        {"file.4k-", 0},
        {"file.8k-", 0},
        {"file.12k", 0}
}, dir5_table[] = {
        {"file.12k+", 0}
};

int test_filler(void *ptr, const char *name,
                const struct stat *st, off_t off)
{
    char **s = ptr;

    s[off] = malloc(sizeof(char) * 28);
    strcpy(s[off], name);

    return 0;
}
int check_dir_table(struct dir_name *table, int row, char **s) {
    for (int i = 0; i < 128; i++) {
        if (s[i]) {
            int seen = 0;
            for (int j = 0; j < row; j++) {
                if (strcmp(table[j].name, s[i]) == 0 && table[j].seen == 0) {
                    table[j].seen = 1;
                    seen = 1;
                }
            }
            if (seen == 0) {
                return -1;
            }
        }
    }

    for (int i = 0; i < row; i++) {
        if (table[i].seen == 0) return -1;
    }
    return 0;
}
void reset_table(struct dir_name *table, int row) {
    for (int i = 0; i < row; i++) {
        table[i].seen = 0;
    }
}
START_TEST(fs_readdir_test)
    {

        char **s = (char**)malloc(sizeof(char*) * 128);
        for (int i = 0; i < 128; i++) s[i] = NULL;
        int rv;
        rv = fs_ops.readdir("/", s, test_filler, 0, NULL);
        ck_assert(rv >= 0);
        ck_assert_int_eq(check_dir_table(dir1_table, 6, s), 0);
        reset_table(dir1_table, 6);

        for (int i = 0; i < 128; i++) s[i] = NULL;
        rv = fs_ops.readdir("/dir2", s, test_filler, 0, NULL);
        ck_assert(rv >= 0);
        ck_assert_int_eq(check_dir_table(dir2_table, 2, s), 0);
        reset_table(dir2_table, 2);

        for (int i = 0; i < 128; i++) s[i] = NULL;
        rv = fs_ops.readdir("/dir3", s, test_filler, 0, NULL);
        ck_assert(rv >= 0);
        ck_assert_int_eq(check_dir_table(dir3_table, 2, s), 0);
        reset_table(dir3_table, 2);

        for (int i = 0; i < 128; i++) s[i] = NULL;
        rv = fs_ops.readdir("/dir3/subdir", s, test_filler, 0, NULL);
        ck_assert(rv >= 0);
        ck_assert_int_eq(check_dir_table(dir4_table, 3, s), 0);
        reset_table(dir4_table, 3);

        for (int i = 0; i < 128; i++) s[i] = NULL;
        rv = fs_ops.readdir("/dir-with-long-name", s, test_filler, 0, NULL);
        ck_assert(rv >= 0);
        ck_assert_int_eq(check_dir_table(dir5_table, 1, s), 0);
        reset_table(dir5_table, 1);

    }
END_TEST
struct {
    unsigned cksum;/* UNSIGNED. TESTS WILL FAIL IF IT'S NOT */
    int len;
    char *path;
} file_table[] = {
        {1786485602, 1000, "/file.1k"},
        {855202508, 10, "/file.10"},
        {4101348955, 12289, "/dir-with-long-name/file.12k+"},
        {2575367502, 1000, "/dir2/twenty-seven-byte-file-name"},
        {799580753, 4098, "/dir2/file.4k+"},
        {4220582896, 4095, "/dir3/subdir/file.4k-"},
        {4090922556, 8190, "/dir3/subdir/file.8k-"},
        {3243963207, 12288, "/dir3/subdir/file.12k"},
        {2954788945, 12287, "/dir3/file.12k-"},
        {2112223143, 8195, "/file.8k+"}
};
START_TEST(fs_read_test)
    {
        ck_assert_int_eq(1, 1);
        // big read
        for (int i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++) {
            char *c = calloc(15000, sizeof(char));

            ck_assert_int_ge(fs_ops.read(file_table[i].path, c, file_table[i].len, 0, NULL), 0);
            unsigned cksum = crc32(0, (Bytef*)c, file_table[i].len);
            ck_assert_int_eq(cksum, file_table[i].cksum);

            free(c);
        }
        // small read
        int size[] = {17, 100, 1000, 1024, 1970, 3000};
        for (int i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++) {
            for (int j = 0; j < 6; j++) {
                int cnt = file_table[i].len / size[j];
                int mod = file_table[i].len % size[j];
                char *c = calloc(15000, sizeof(char));

                for (int z = 0; z < cnt; z++) {
                    ck_assert_int_ge(fs_ops.read(file_table[i].path, c, size[j], z * size[j], NULL), 0);
                }

                ck_assert_int_ge(fs_ops.read(file_table[i].path, c, mod, cnt * size[j], NULL), 0);
                unsigned cksum = crc32(0, (Bytef*)c, file_table[i].len);
                ck_assert_int_eq(cksum, file_table[i].cksum);
                free(c);
            }
        }

    }
END_TEST

START_TEST(fs_rename_test)
    {
        // rename a file
        ck_assert_int_eq(fs_ops.rename("/file.10", "/new_name.10"), 0);
        char *c = calloc(15000, sizeof(char));
        ck_assert_int_gt(fs_ops.read("new_name.10", c, file_table[1].len, 0, NULL), 0);
        // rename a directory
        ck_assert_int_eq(fs_ops.rename("/dir2", "/new_dir2"), 0);
        char *c1 = calloc(15000, sizeof(char));
        ck_assert_int_gt(fs_ops.read("/new_dir2/file.4k+", c1, file_table[4].len, 0, NULL), 0);
    }
END_TEST

START_TEST(fs_chmod_test)
    {
        // change file's mode
        struct stat *sb = malloc(sizeof(const struct stat));
        fs_ops.chmod("/file.8k+", 0777);
        fs_ops.getattr("/file.8k+", sb);
        ck_assert_int_eq(sb->st_mode, 33279);

        // change directory's mode;
        struct stat *sb1 = malloc(sizeof(const struct stat));
        fs_ops.chmod("/dir-with-long-name", 0777);
        fs_ops.getattr("/dir-with-long-name", sb1);
        ck_assert_int_eq(sb1->st_mode, 16895);

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

//    tcase_add_test(tc, fs_getattr_test);
//    tcase_add_test(tc, fs_readdir_test);
    tcase_add_test(tc, fs_read_test);
//    tcase_add_test(tc, fs_statfs_test);
//    tcase_add_test(tc, fs_rename_test);
//    tcase_add_test(tc, fs_chmod_test);



    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    int n_failed = srunner_ntests_failed(sr);
    printf("%d tests failed\n", n_failed);
    
    srunner_free(sr);
    return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
