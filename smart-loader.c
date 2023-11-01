#include "loader.h"
#include <signal.h>
#include <errno.h>

#define PAGE_SIZE 4096

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;

int fd, segment = 0;
void *virtual_mem;
int offset_vmem;

void sigsegv_handler(int signum, siginfo_t *info, void *context){

      printf("SIGSEGV invoked!\n");

      void* fault_addr = info->si_addr;
      int pages = (phdr[segment].p_memsz)/PAGE_SIZE + 1;


      printf("This is the memsz %d\n", phdr[segment].p_memsz);
      printf("This is the p_vaddr: %d\n",phdr[segment].p_vaddr);
      printf("This is the p_offset: %d\n",phdr[segment].p_offset);
      
      printf("pages: %d\n", pages);
      printf("Internal fragmentation: %d\n", pages*PAGE_SIZE - phdr[segment].p_memsz);

      virtual_mem = mmap((void*)phdr[segment].p_vaddr, PAGE_SIZE*pages, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, phdr[segment].p_offset);

      lseek(fd , phdr[segment].p_offset , SEEK_SET);
      read(fd , virtual_mem , PAGE_SIZE*pages);

      if(virtual_mem == MAP_FAILED){
        printf("Memory size: %d\n",phdr->p_memsz);
        printf("Memory location: %d\n",phdr->p_vaddr);
        printf("Page size: %d\n",PAGE_SIZE*pages);
        printf("Offset: %d\n", phdr->p_offset);
        perror("mmap");
        exit(4);
      }
      segment++;
}

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Couldn't open file");
        exit(0);
    }
    
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    read(fd,ehdr,sizeof(Elf32_Ehdr));
    
    lseek(fd, (ehdr->e_phoff), SEEK_SET);
    phdr = (Elf32_Phdr*)realloc(phdr,sizeof(Elf32_Phdr)*ehdr->e_phnum);
    read(fd,phdr,sizeof(Elf32_Phdr)*ehdr->e_phnum);

    int (*_start)() = (int(*)())(ehdr->e_entry);
    int result = _start();
    printf("User _start return value = %d\n", result);

    free(ehdr);
    free(phdr);
}

int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    load_and_run_elf(argv);
    return 0;
}