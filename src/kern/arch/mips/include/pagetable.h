#ifndef _MIPS_PT_H_
#define _MIPS_PT_H_


typedef struct page_table_entry{
    // write this
    vaddr_t virtual_addr;
    paddr_t physical_addr;
} page_table_entry;


typedef int size_page_table;

const int MAX_PT_SZ =2048;
typedef struct page_table{
    pid_t pid;                              // the process having this information
    size_page_table size;                   // number of entries
    page_table_entry table[MAX_PT_SZ];      // address space

    bool present_bit[MAX_PT_SZ];
    bool dirty_bit[MAX_PT_SZ];
} page_table;


// add new entry
int add_page_entry(page_table tbl&, page_table_entry pentr);

// change existing entry. pentr must have the intended physical_addr for the previous virtual_addr;
int change_page_entry(page_table tbl&, page_table_entry pentr);

// remove the entry
int delete_page_entry(page_table tbl&, vaddr_t vaddr);

// initiate the page table
int page_table_bootstrap(page_table tbl&);
#endif
