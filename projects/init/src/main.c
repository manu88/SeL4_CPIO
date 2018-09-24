#include <stdio.h>
#include <cpio/cpio.h>
#include <sel4platsupport/bootinfo.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <sel4utils/vspace.h>

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char _cpio_archive[];


#define APP_NAME "app"


/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];


/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

simple_t simple;
allocman_t *allocman;
vka_t vka;
vspace_t vspace;

int main(void)
{
    printf("init started\n");


    seL4_BootInfo *boot_info = platsupport_get_bootinfo();



    /* init simple */
    simple_default_init_bootinfo(&simple, boot_info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);


    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
                                            allocator_mem_pool);

    
    if( allocman == NULL)
    {	 
	printf("Failed to initialize allocator.\n \tMemory pool sufficiently sized?\n \tMemory pool pointer valid?\n");
        return 2;
    }


    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);


    UNUSED int error = 0;

    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky( &vspace , &data ,simple_get_pd(&simple)  ,&vka, boot_info); 

    if(error != 0)
    {
	printf( "Failed to prepare root thread's VSpace for use.\n"
                  "\tsel4utils_bootstrap_vspace_with_bootinfo reserves important vaddresses.\n"
                  "\tIts failure means we can't safely use our vaddrspace.\n");
	return 3;
    }
/*
    unsigned long elf_size;

    char* elf_base = cpio_get_file(_cpio_archive, APP_NAME, &elf_size);
    
    if (elf_base == NULL) 
    {
        printf("Unable to locate cpio header for %s", APP_NAME);
        return 1;
    }

    printf("CPIO Size : %lu\n", elf_size);
*/
    return 0;
}
