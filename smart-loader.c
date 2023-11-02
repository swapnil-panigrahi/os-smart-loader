#include "loader.h"
#include <signal.h>
#include <errno.h>

#define PAGE_SIZE 4096

typedef struct segment_address{
    Elf32_Addr fault_address;
    size_t size;
}segment_address;

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd, entrypoint = -1;
int page_faults = 0, page_allocations = 0, total_fragmentation = 0, segment = 0, fault_index = 0;
segment_address seg_addr[100];
void *virtual_mem;

int find_segment(void* addr){
    for (int i=0; i<fault_index; i++){
        if (seg_addr[i].fault_address <= (uintptr_t) addr && (uintptr_t) addr <= seg_addr[i].fault_address + seg_addr[i].size){
            return i;
        }
    }
    return -1;
}

void sigsegv_handler(int signum, siginfo_t *info, void *context) {
    page_faults++;
    void* fault_addr = info->si_addr;

    // printf("Fault address: %p\n", fault_addr);
    int pages = (phdr[segment].p_memsz) / PAGE_SIZE;
    if (phdr[segment].p_memsz % PAGE_SIZE != 0) {
        pages++;
    }
    page_allocations += pages;
    
    int fault_segment_index = find_segment(fault_addr);
    // printf("Faulty segment: %d\n", fault_segment_index);
    if (fault_segment_index!=-1){
        int fault_pages = seg_addr[fault_segment_index].size/PAGE_SIZE;
        if (fault_pages * PAGE_SIZE != seg_addr[fault_segment_index].size){
            fault_pages++;
        }
        total_fragmentation += PAGE_SIZE * fault_pages - phdr[segment].p_memsz;
    }
    for (int i=0; i<ehdr->e_phnum; i++){
        if (phdr[i].p_vaddr <= (uintptr_t) fault_addr && (uintptr_t) fault_addr <= phdr[i].p_vaddr + phdr[i].p_memsz){
            seg_addr[fault_index].fault_address = phdr[i].p_vaddr;
            seg_addr[fault_index].size = phdr[i].p_memsz;
        }
    }
    fault_index++;

    // printf("Segment size: %d\n", phdr[segment].p_memsz);
    // printf("Segment address: %p\n", phdr[segment].p_vaddr);
    // printf("Internal Fragmentation: %d\n", pages*PAGE_SIZE - phdr[segment].p_memsz);

    int flags = MAP_PRIVATE | MAP_FIXED;
    if (segment != entrypoint) {
        flags |= MAP_ANONYMOUS;
    }

    virtual_mem = mmap((void *)phdr[segment].p_vaddr, PAGE_SIZE * pages,
                      PROT_READ | PROT_WRITE | PROT_EXEC, flags, fd, phdr[segment].p_offset);

    if (virtual_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    segment++;
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