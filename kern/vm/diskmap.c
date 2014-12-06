#include <diskmap.h>
#include <types.h>
#include <lib.h>
#include <vm.h>
#include <spinlock.h>
#include <bitmap.h>
#include <coremap.h>



// allocate a static spinlock
static struct spinlock diskmap_lock;

struct bitmap* diskmap;


// acquires the diskmap lock
void dm_acquire_lock(void){
    spinlock_acquire(&diskmap_lock);
}

// releases the diskmap lock
void dm_release_lock(void){
    spinlock_release(&diskmap_lock);
}

// returns true if the page on disk is not occupied, false otherwise
bool dm_is_free(unsigned long page_index){ 
    KASSERT(page_index < diskmap->nbits);

    return bitmap_isset(diskmap, page_index);
}



// sets the specified page to free
void dm_set_free(unsigned long page_index) {
    KASSERT(page_index < diskmap->nbits);


    if(is_free(page_index)) {
        // page is already free. // TDOD: is this an error?
        // KASSERT(false);
    }

    bitmap_unmark(diskmap, page_index);
}


// sets the specified page to occupied
void dm_set_occupied(unsigned long page_index){ 
    KASSERT(page_index < diskmap->nbits);
    
    if(!is_free(page_index)) {
        // page is already in use
        KASSERT(false);
    }

    bitmap_mark(diskmap, page_index);
}


// get the index of a free page. page will be marked as occupied. returns false if disk full
bool dm_get_free_page(unsigned long* page_index){
    KASSERT(page_index != NULL);

    for(unsigned long i = 0; i < diskmap->nbits; i++){
        if(is_free(i)){
            *page_index = i;

            dm_set_occupied(i);

            return true;
        }
    }

    *page_index = 0;
    return false;
}




// sets up the space in virtual memory to hold the diskmap 
// !!! We call alloc_kpages several times to get contiguous memory.
// !!! Since bootstrap is called directly after the coremap bootstrap we assume we get contiguous memory
void diskmap_bootstrap(void){

    // get the number of available pages
    unsigned int number_of_pages_avail = get_coremap_size();

    // number of available pages upscaled
    unsigned long number_of_disk_pages = number_of_pages_avail * 16;

    // calculate number of pages needed to store the diskmap (bits and bitmap struct)
    unsigned int number_of_pages = DIVROUNDUP(DIVROUNDUP(number_of_disk_pages, 8) + (unsigned int)sizeof(struct bitmap), PAGE_SIZE); 

    // 512 MB memory max
    // 4096 Byte a page
    // -> 131072 pages
    // pages needed for swapping = bits needed for diskmap
    // 131072 pages * 16 = 2097152 bits
    //                   = 262144 Byte
    //                   = 256 k Byte
    //                                  -> 64 pages max

    // we assume that we can allocate gontiguous pages since we call this function right after coremap_bootstrap()
    // allocate at least one page for the bitmap
    diskmap = (struct bitmap*) alloc_kpages(1);
    KASSERT(diskmap != NULL);
    for(unsigned int i = 1; i < number_of_pages; i++) {
        vaddr_t ret = alloc_kpages(1);
        KASSERT(ret != (vaddr_t)NULL);
    }
    
    (void) number_of_pages;

    // create bitmap
    bitmap_create_diskmap(diskmap, number_of_disk_pages);
}








