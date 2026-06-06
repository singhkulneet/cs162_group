#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

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
  if (!valid_user_byte(word_addr) || !valid_user_byte(word_addr + 3))
    exit_invalid();
}

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame* f) {
  /* Validate the syscall BEFORE reading it */
  validate_word((uint32_t*)f->esp);
  uint32_t* args = ((uint32_t*)f->esp);
  struct thread* t = thread_current();
  struct process* pcb = t->pcb;

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  switch (args[0]) {
    case SYS_HALT: // no args, no return
      shutdown_power_off();
      break;

    case SYS_EXIT:
      validate_word(&args[1]); /* exit status */
      f->eax = args[1];
      printf("%s: exit(%d)\n", pcb->process_name, args[1]);
      process_exit();
      break;

    case SYS_EXEC:
      validate_word(&args[1]);           /* cmd address */
      validate_word((uint32_t*)args[1]); /* cmd string */
      {
        const char* cmd_line = (char*)args[1];
        f->eax = process_execute(cmd_line);
      }
      break;

    case SYS_WAIT:
      validate_word(&args[1]); /* pid */
      { f->eax = process_wait(args[1]); }
      break;

    case SYS_CREATE:                     // Creates a new file called file initially initial_size
      validate_word(&args[1]);           /* file address */
      validate_word((uint32_t*)args[1]); /* file string */
      validate_word(&args[2]);           /* intial size */
      {
        const char* file = (char*)args[1];
        int32_t initial_size = (int32_t)args[2];
        f->eax = filesys_create(file, initial_size);
      }
      break;

    case SYS_REMOVE:                     // Closes an existing file (if it exists)
      validate_word(&args[1]);           /* file address */
      validate_word((uint32_t*)args[1]); /* file string */
      {
        const char* file = (char*)args[1];
        f->eax = filesys_remove(file);
      }
      break;

    case SYS_OPEN:                       // Opens an existing file (if it exists)
      validate_word(&args[1]);           /* file address */
      validate_word((uint32_t*)args[1]); /* file string */
      {
        const char* file = (char*)args[1];
        if (pcb->fd_size >= MAX_FILES) {
          f->eax = -1;
          return;
        }
        struct file* fp = filesys_open(file);
        if (fp) {
          pcb->fd_table[pcb->fd_size] = fp;
          f->eax = pcb->fd_size++;
        } else {
          f->eax = -1;
        }
      }
      break;

    case SYS_FILESIZE:
      validate_word(&args[1]); /* fd */
      {
        int fd = args[1];
        int size = -1;
        if (fd < pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp)
            size = file_length(fp);
        }
        f->eax = size;
      }
      break;

    case SYS_READ:
      validate_word(&args[1]);           /* fd */
      validate_word(&args[2]);           /* buf pointer */
      validate_word((uint32_t*)args[2]); /* user buf pointer */
      validate_word(&args[3]);           /* size */
      {
        int fd = args[1];
        char* buf = (char*)args[2];
        int size = args[3];
        int read = -1;
        if (fd == STDIN_FILENO) {
          read = 0;
          while (read < size)
            buf[read++] = input_getc();
        } else if (fd < pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp)
            read = file_read(fp, buf, size);
        }
        f->eax = read;
      }
      break;

    case SYS_WRITE:
      validate_word(&args[1]);           /* fd */
      validate_word(&args[2]);           /* buf pointer */
      validate_word((uint32_t*)args[2]); /* buf pointer */
      validate_word(&args[3]);           /* size */
      {
        int fd = args[1];
        char* buf = (char*)args[2];
        int size = args[3];
        int wrote = -1;
        if (fd == STDOUT_FILENO) {
          putbuf(buf, size); // TODO: make multiple calls if size too large
          wrote = size;
        } else if (fd < (int)pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp)
            wrote = file_write(fp, buf, size);
        }
        f->eax = wrote;
      }
      break;

    case SYS_SEEK:
      validate_word(&args[1]); /* fd */
      validate_word(&args[2]); /* position */
      {
        int fd = args[1];
        unsigned position = args[2];
        if (fd < (int)pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp)
            file_seek(fp, position);
        }
      }
      break;

    case SYS_TELL:
      validate_word(&args[1]); /* fd */
      {
        int fd = args[1];
        int position = -1;
        if (fd < (int)pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp)
            position = file_tell(fp);
        }
        f->eax = position;
      }
      break;

    case SYS_CLOSE:
      validate_word(&args[1]); /* fd */
      {
        int fd = args[1];
        if (fd < (int)pcb->fd_size && fd > STDOUT_FILENO) {
          struct file* fp = pcb->fd_table[fd];
          if (fp) {
            file_close(fp);
            pcb->fd_table[fd] = NULL;
          }
        }
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
