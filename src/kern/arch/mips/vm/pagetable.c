#include <pagetable.h>


// write these
int add_page_entry(page_table tbl&, page_table_entry pentr){
    if(size==MAX_PT_SZ) return 1;   // not found
    table[size]=pentr;
    present_bit[size]=true;
    dirty_bit[size]=false;
    modified_bit[size]=false;
    size++;
    return 0;
}

int change_page_entry(page_table tbl&, page_table_entry pentr){
    int i=0;
    for(;i<size;i++){
        if(pentr.virtual_addr==table[i].virtual_addr){
            table[i].physical_addr=pentr.physical_addr;
            return 0;
        }
    }
    // not found
    return 1;
}


int delete_page_entry(page_table tbl&, vaddr_t vaddr){
    // TODO : check if dirty, and perform a swap out if it is

    int i=0;
    for(;i<size;i++){
        if(pentr.virtual_addr==table[i].virtual_addr){
            table[i].physical_addr=pentr.physical_addr;
            present_bit[i]=false;
            return 0;
        }
    }
    // not found
    return 1;
}

int page_table_bootstrap(page_table tbl&){
    tbl.size=0;
    return 0;
}
