#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame*);

/* Terminate the current process with exit code -1. */
static void exit_invalid(void) {
  printf("%s: exit(%d)\n", thread_current()->pcb->process_name, -1);
  process_exit();
  NOT_REACHED();
}

/* Return true if ADDR is a valid, mapped user-space byte */
static bool valid_user_byte(const void* addr) {
  return addr != NULL && is_user_vaddr(addr) &&
         pagedir_get_page(thread_current()->pcb->pagedir, addr) != NULL;
}

/* Validate that a 4-byte word at WORD_ADDR is fully in mapped user memory */
static void validate_word(const uint32_t* word_addr) {
  if (!valid_user_byte(word_addr) || !valid_user_byte((const char*)word_addr + 3))
    exit_invalid();
}

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame* f) {
  /* Validate the syscall BEFORE reading it */
  validate_word((uint32_t*)f->esp);
  uint32_t* args = ((uint32_t*)f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  switch (args[0]) {
    case SYS_EXIT:
      validate_word(&args[1]); /* exit status */
      f->eax = args[1];
      printf("%s: exit(%d)\n", thread_current()->pcb->process_name, args[1]);
      process_exit();
      break;

    case SYS_WRITE:
      validate_word(&args[1]); /* fd */
      validate_word(&args[2]); /* buf pointer */
      validate_word(&args[3]); /* size */
      {
        int fd = args[1];
        char* buf = (char*)args[2];
        uint32_t size = args[3];
        if (fd == STDOUT_FILENO) {
          putbuf(buf, size); // TODO: make multiple calls if size too large
        }
        f->eax = size;
      }
      break;

    case SYS_PRACTICE:
      validate_word(&args[1]); /* integer argument */
      f->eax = (int)args[1] + 1;
      break;

    default:
      break;
  }
}
