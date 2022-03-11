#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);
    mem[0] = 1;
}

//
// Allocate a physical page
//
// Returns the number of the page, or 0xff if no more pages available
//
unsigned char get_page(void)
{
    // For each page_number inside the first 64 bytes of the zero page (the used page array)    
    for(int i = 1; i < 64; i++) {
        if (mem[i] == 0) {
            mem[i] = 1;
            return i;
        }
    }
    return 0xff;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    unsigned char page_table = get_page();
    if (page_table == 0xff) {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    mem[PAGE_COUNT + proc_num] = page_table;
    
    for(int i = 0; i < page_count; i++) {
        unsigned char new_page = get_page();
        if (new_page == 0xff) {
            printf("OOM: proc %d: page table\n", proc_num);
            return;
        }
        int pt_address = get_address(page_table, i);
        mem[pt_address] = new_page;
    }
}

//
// Get the page table for a given process
//
unsigned char get_page_table(int proc_num)
{
    return mem[proc_num + PAGE_COUNT];
}

void dealloc_page(int proc_num) {
    mem[proc_num] = 0;
}

void kill_process(int proc_num) {
    unsigned char page_table = get_page_table(proc_num);

    for (int i = 0; i < PAGE_SIZE; i++) {
        int page_addr = get_address(page_table, i);

        if(mem[page_addr]) {
            dealloc_page(mem[page_addr]);
            mem[page_addr] = 0;
        }
    }
    dealloc_page(page_table);

}

int get_phys_addr(int proc_num, int virtual_addr) {
    int virtual_page = virtual_addr >> 8;
    int offset = virtual_addr & 255;
    int process_pt_addr = get_address(get_page_table(proc_num), virtual_page);
    unsigned char phys_page = mem[process_pt_addr];
    
    return get_address(phys_page, offset);
}

void store_value(int proc_num, int virtual_addr, int value) {
    int phys_addr = get_phys_addr(proc_num, virtual_addr);
    mem[phys_addr] = value;

    printf("Store proc %d: %d => %d, value=%d\n", proc_num, virtual_addr, phys_addr, value);
}

void load_value(int proc_num, int virtual_addr) {
    int phys_addr = get_phys_addr(proc_num, virtual_addr);
    unsigned char value = mem[phys_addr];

    printf("Load proc %d: %d => %d, value=%d\n", proc_num, virtual_addr, phys_addr, value);
}

//
// Print the free page map
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int pages = atoi(argv[++i]);
            new_process(proc_num, pages);
        }
        else if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "kp") == 0) {
            int proc_num = atoi(argv[++i]);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int virtual_addr = atoi(argv[++i]);
            int value = atoi(argv[++i]);
            store_value(proc_num, virtual_addr, value);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int virtual_addr = atoi(argv[++i]);
            load_value(proc_num, virtual_addr);
        }
    }
}
