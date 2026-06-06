/* Try writing a file in the most normal way. */

#include <syscall.h>
#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void test_main() {
  int handle, byte_cnt;
  char* t_file = "test.txt";

  CHECK(create(t_file, sizeof sample - 1), "create \"%s\"", t_file);
  CHECK((handle = open(t_file)) > 1, "open \"%s\"", t_file);

  byte_cnt = write(handle, sample, sizeof sample - 1);
  CHECK(byte_cnt == sizeof sample - 1, "wrote %d bytes to \"%s\"", byte_cnt, t_file);
  CHECK(filesize(handle) == byte_cnt, "Check filesize = %d \"%s\"", byte_cnt,
        t_file); // check file size

  CHECK((int)tell(handle) == byte_cnt, "Tell is at %d \"%s\"", byte_cnt,
        t_file);   // should be at byte_cnt after write
  seek(handle, 0); // seek to beginning
  CHECK(tell(handle) == 0, "Tell is at 0 after seek 0 \"%s\"", t_file); // reset to beginning
  check_file_handle(handle, t_file, sample, sizeof sample - 1); // check read to verify contents
  close(handle);
  CHECK(filesize(handle) == -1, "Check filesize is -1 after close \"%s\"",
        t_file);                                  // check file size
  CHECK(remove(t_file), "remove \"%s\"", t_file); // remove file
  CHECK(open(t_file) == -1, "open \"%s\" failed after remove",
        t_file); // should not be able to open removed file
}
