
#define NUMFRAMES 1024

//pageframe contains a frame id, a bit showing whether or not it contains an actual page and a pointer to the next pageframe
typedef struct pageframe
{
  int frame_id;
  bool valid_bit;
  uint32_t paddr;
  int pid;
  pageframe *next;
} pageframe;

//frame table is an array of frames
pageframe pframetable[NUMFRAMES];

//the find_free_frame function returns the id of first empty frame (if any).. if all frames are //occupied, it returns -1
int find_free_frame()
{
  int i;
  int free=-1;
  for(i=0;i<NUMFRAMES;i++)
{
      if(!pframetable[i].valid_bit)

{
          free=i;
          break;
      }
}
  return free;
}

void bring_to_mem(uint_32 swap_paddr)
{
//create space for a page using kmalloc
  page pnew = kmalloc(SIZEOFPAGE);
//use the address from swap_paddr and write from the swapped page into the new page
  write_into_page(pnew, swap_paddr);
// get the id for a free frame
  int x=find_free_frame();
  if(x==-1)  
{
//if free frame not found, remove some page from memory using the replacement policy and find free frame again
      remove_page(); //using relevant replacement policy
      x=find_free_frame();
}
  pframetable[x].valid_bit=1;
  pframetable[x].paddr=&pnew;
}
