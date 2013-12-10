extern clock_angle;
extern size_page_table next_to_be_replaced;
extern size_page_table rear_hand = 0;
extern size_page_table front_hand = CLOCK_ANGLE;
extern size_pft clock_hand = 0;
enum page_replacement_policy {LRU, FIFO, Random, One_Handed, Second_Chance};

paddr_t lru_replace(page_table* pt){
    size_page_table sz = pt->size; //size_page_table is a typedef : such as int : depicts number of entries in page_table
    size_page_table i;
    size_page_table selected_page_index = 0;
    time_stamp t_min = pt->array[0].t_stamp;
    for(i=1; i<sz; i++){
        if(t_min>pt->array[i].t_stamp){
            t_min = pt->array[i].t_stamp;
            selected_page_index = i;
        }
    }
    
    return pt->array[selected_page_index].physical_addr;
}

paddr_t fifo_replace(page_table* pt){
    return pt->array[next_to_be_replaced++].physical_addr;
}

paddr_t random_replace(page_table* pt){
    size_page_table r = rand(0, pt->size - 1);
    return pt->array[r].physical_addr;
}

paddr_t second_chance_replace(){
    size_pft num_page_frames = MAX_PM_SIZE;
    size_pft i = 0;
    for(i; i<=clock_angle; i++){
        if(pft.table[rear_hand].ref_bit == false){
                break;
        }
        else{
            pft.table[front_hand].ref_bit = false;
            rear_hand++;
            front_hand++;
            rear_hand%=sz;
            front_hand%=sz;
        }
    }
    
    
    paddr_t new_page_frame = pft.table[rear_hand%num_page_frames].physical_addr;
    addrspace* p_space_old = pft.table[new_page_frame].p_space;
    vaddr_t old_page = pft.table[new_page_frame].virtual_addr;
    
    vaddr_t old_page_lvl_1 = LEVEL1INDEX(old_page);
    vaddr_t old_page_lvl_2 = LEVEL2INDEX(old_page);
    
    p_space_old->table[old_page_lvl_1]->table[old_page_lvl_2].valid_bit = false;
    
    return new_page_frame;
}

typedef int size_pft;

paddr_t one_handed_replace(){
    size_pft num_page_frames = MAX_PM_SIZE;
    for(int i=0; i<num_page_frames; i++){
        if(pft.table[clock_hand].ref_bit == false){
            break;
        }
        else {
            pft.table[clock_hand].ref_bit = false;
            clock_hand++;
            clock_hand %= num_page_frames;
        }
    }
    
    paddr_t new_page_frame = pft.table[clock_hand%num_page_frames].physical_addr;
    addrspace* p_space_old = pft.table[new_page_frame].p_space;
    vaddr_t old_page = pft.table[new_page_frame].virtual_addr;
    
    vaddr_t old_page_lvl_1 = LEVEL1INDEX(old_page);
    vaddr_t old_page_lvl_2 = LEVEL2INDEX(old_page);
    
    p_space_old->table[old_page_lvl_1]->table[old_page_lvl_2].valid_bit = false;
    
    return new_page_frame;
}
