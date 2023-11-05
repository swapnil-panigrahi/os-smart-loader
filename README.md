# os-smart-loader
## Functionality
- The os-smart-loader executes files generated after the linking process is done by the linker.
- It only accepts Elf32 files.
- It has its own custom memory allocation system which allocates memory based on which segment is assessed.
- It allocates the required memory to resolve the page fault.
- It also keeps track of the number of Page Faults that occur as well as the memory lost due to fragmentation.
- It prints all the collected info in the terminal when used.
## Implementation(without bonus)
- First, there is no memory allocated to any segment of the program.
- When the program is executed a segmentation Fault occurs because even thought the virtual addresses are valid no physical memory is associated with them at this time.
- This segmentation fault then triggers the custom `sigsegv_handler` made for the program.
- Inside the `sigsegv_handler` the segment in which the fault happened is calculated alongside which the number of pages to be allocated as well as the memory loss due to fragmentation is also calculated.
- After all the necessary info has been calculated the required number of pages are allocated to the segment using mmap.
- If for some reason mmap fails the program is exited and the error is printed.
- ## Implementation(bonus)
- First, there is no memory allocated to any segment of the program.
- When the program is executed a segmentation Fault occurs because even thought the virtual addresses are valid no physical memory is associated with them at this time.
- This segmentation fault then triggers the custom `sigsegv_handler` made for the program.
- Inside the `sigsegv_handler` the page in which the fault happened is calculated alongside which the offset of that page as well as the memory loss due to fragmentation is also calculated.
- After all the necessary info has been calculated a page is allocated to the page's virtual address using mmap.
- For some reason if mmap fails the program is exited and the error is printed.
## Contributions
- Implementing smart-loader where the memory allocation is done of an entire segment whenever a page fault occurs (Working), Code Cleanup and Munmap - Swapnil Panighari.
- Implementing smart-loader where the memory allocation is done page wise (Working) and Makefile - Rohak Kansal.
## GitHub Repository
- https://github.com/swapnil-panigrahi/os-smart-loader
