#include "loader.h"
#include <signal.h>
#include <errno.h>

#define PAGE_SIZE 4096

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd, entrypoint = -1;
int page_faults = 0, page_allocations = 0, total_fragmentation = 0, segment = 0;
void *page_allocated;


void sigsegv_handler(int signum, siginfo_t *info, void *context) {
    page_faults++;
    void* fault_addr = info->si_addr;
    printf("Fault address: %u\n", fault_addr);
    // calculating segment in which page fault occurred
    printf("jhskjadhlkjf:%d\n",ehdr->e_phnum);
    int i = 0;
    while(i < ehdr->e_phnum) {
        Elf32_Addr seg_start = phdr[i].p_vaddr;
        Elf32_Addr seg_end = seg_start + phdr[i].p_memsz;
        printf("start: %u\nend: %u\n",seg_start,seg_end);
        if((uintptr_t)fault_addr >= seg_start && (uintptr_t)fault_addr <= seg_end){
            break;
        }
        i++;
    }
    // calculating address where page is needed to be inserted
    int fault_seg_index = i;
    Elf32_Addr seg_page_addr = phdr[fault_seg_index].p_vaddr;
    while(seg_page_addr <= fault_addr){
        seg_page_addr = (void*) ((uintptr_t) seg_page_addr + PAGE_SIZE);
    }
    seg_page_addr = (void*) ((uintptr_t) seg_page_addr - PAGE_SIZE);
    lseek(fd,seg_page_addr,SEEK_SET);
    // debugging start
    printf("Faulty segment index: %d\n", fault_seg_index);
    printf("Faulty page: %u\n", seg_page_addr);
    printf("Faulty segment start: %u\n", phdr[fault_seg_index].p_vaddr);
    printf("Faulty segment end: %u\n", phdr[fault_seg_index].p_vaddr + phdr[fault_seg_index].p_memsz);
    // debugging end
    // calculating fragmentation
    uintptr_t overshoot = (uintptr_t) seg_page_addr + PAGE_SIZE;
    if(overshoot > (uintptr_t) phdr[fault_seg_index].p_vaddr + phdr[fault_seg_index].p_memsz)
        overshoot -= (uintptr_t) phdr[fault_seg_index].p_vaddr + phdr[fault_seg_index].p_memsz;
    else overshoot = 0;
    total_fragmentation += overshoot;
    if (seg_page_addr == 0x0){
        page_allocated = mmap((void*)seg_page_addr, PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, phdr[fault_seg_index].p_offset);
        if (page_allocated == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        return;
    }
    // allocating the page to resolve the page fault
    page_allocated = mmap((void*)seg_page_addr, PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    // if map fails
    if (page_allocated == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Couldn't open file");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    read(fd, ehdr, sizeof(Elf32_Ehdr));

    lseek(fd, ehdr->e_phoff, SEEK_SET);

    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_vaddr <= ehdr->e_entry && ehdr->e_entry <= phdr[i].p_vaddr + phdr[i].p_memsz) {
            entrypoint = i;
        }
    }

    int (*_start)() = (int (*)())(ehdr->e_entry);
    int result = _start();
    printf("User _start return value = %d\n", result);
    printf("Total page faults: %d\n", page_faults);
    printf("Pages Allocated: %d\n", page_allocations);
    printf("Total fragmentation (in KB): %0.4fKB\n", total_fragmentation/(double) 1024);
}

void loader_cleanup() {
    free(ehdr);
    free(phdr);

}

int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}