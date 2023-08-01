## Storage Manager 

In this assignment, we implemented a simple storage manager that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk. Basically, we implemented all interfaces described in `storage_mgr.h`

## Files

| File             | Description                                            |
| ---------------- | ------------------------------------------------------ |
| `store_mgr.*`    | Responsible for managing database in files and memory. |
| `dberror.*`      | Keeps track and report different types of error.       |
| `test_assign1.c` | Contains main function running all tests.              |
| `test_helper.h`  | Testing and assertion tools.                           |

## Execution Environment 

Before running the whole project, please ensure that your environment has successfully installed `make` and `gcc`. Here is an example. 

```shell
make --version
GNU Make 4.3
Built for x86_64-pc-linux-gnu
Copyright (C) 1988-2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
```

```shell
$ gcc --version
gcc (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
Copyright (C) 2021 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
```

## Compiling and Running

1. Enter into the folder containing all documents. `$ cd /assign1`
2. Execute `$ make`
3. See result s`$ ./test_assign1`


## Extra Testcases

We had added two testcases `static void testMultiplePageContent(void)` and `static void testExpandCapacity(void)` to test what happened when there are multiple page and when the system need to expand capacity. 

```c
// Test multiple page content.
void testMultiplePageContent(void) {
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test Multiple page content";

  ph = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));

  // create a new page file
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("created and opened file\n");

  // add new page
  TEST_CHECK(appendEmptyBlock(&fh));
  printf("Add new page with zero bytes\n");

  // read new page into handle
  TEST_CHECK(readNextBlock(&fh, ph));
  // the page should be empty (zero bytes)
  for (i = 0; i < PAGE_SIZE; i++) {
    ASSERT_TRUE(ph[i] == 0,
                "expected zero byte in new page of freshly initialized page");
  }
  printf("\n new block was empty\n");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  TEST_DONE();
}
```

```c
// Try to increase capacity on a file by a certain number of pages.
void testExpandCapacity(void) {
  SM_FileHandle fh;

  testName = "test expandable capacity";

  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));

  ASSERT_TRUE(fh.totalNumPages == 1, "not expanded yet");
  TEST_CHECK(ensureCapacity(5, &fh));
  ASSERT_TRUE(fh.totalNumPages == 5, "expanded to 5");
  TEST_CHECK(ensureCapacity(10, &fh));
  ASSERT_TRUE(fh.totalNumPages == 10, "expanded again to 10");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  TEST_DONE();
}
```