#include <stdio.h>
#include <cpio/cpio.h>
#include <sel4platsupport/bootinfo.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <sel4utils/vspace.h>


#include <sel4utils/process.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "app"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];


int main(void)
{
    printf("init started\n");
UNUSED int error = 0;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("init:");
    //name_thread(seL4_CapInitThreadTCB, "hello-4");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
                                            allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize allocator.\n"
               "\tMemory pool sufficiently sized?\n"
               "\tMemory pool pointer valid?\n");

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);


/* TASK 1: create a vspace object to manage our vspace */
    /* hint 1: sel4utils_bootstrap_vspace_with_bootinfo_leaky()
     * int sel4utils_bootstrap_vspace_with_bootinfo_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr page_directory, vka_t *vka, seL4_BootInfo *info)
     * @param vspace Uninitialised vspace struct to populate.
     * @param data Uninitialised vspace data struct to populate.
     * @param page_directory Page directory for the new vspace.
     * @param vka Initialised vka that this virtual memory allocator will use to allocate pages and pagetables. This allocator will never invoke free.
     * @param info seL4 boot info
     * @return 0 on succes.
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_1:
     */

    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky( &vspace , &data ,simple_get_pd(&simple)  ,&vka, info); 
    ZF_LOGF_IFERR(error, "Failed to prepare root thread's VSpace for use.\n"
                  "\tsel4utils_bootstrap_vspace_with_bootinfo reserves important vaddresses.\n"
                  "\tIts failure means we can't safely use our vaddrspace.\n");

    /* fill the allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
                                               ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    ZF_LOGF_IF(virtual_reservation.res == NULL, "Failed to reserve a chunk of memory.\n");
    bootstrap_configure_virtual_pool(allocman, vaddr,
                                     ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));


    /* TASK 2: use sel4utils to make a new process */
    /* hint 1: sel4utils_configure_process_custom()
     * hint 2: process_config_default_simple()
     * @param process Uninitialised process struct.
     * @param vka Allocator to use to allocate objects.
     * @param vspace Vspace allocator for the current vspace.
     * @param priority Priority to configure the process to run as.
     * @param image_name Name of the elf image to load from the cpio archive.
     * @return 0 on success, -1 on error.
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_2:
     *
     * hint 2: priority is in APP_PRIORITY and can be 0 to seL4_MaxPrio
     * hint 3: the elf image name is in APP_IMAGE_NAME
     */

    sel4utils_process_t process;
    sel4utils_process_config_t config = process_config_default_simple( &simple, APP_IMAGE_NAME, APP_PRIORITY);
    error = sel4utils_configure_process_custom( &process , &vka , &vspace, config);

    ZF_LOGF_IFERR(error, "Failed to spawn a new thread.\n"
                  "\tsel4utils_configure_process expands an ELF file into our VSpace.\n"
                  "\tBe sure you've properly configured a VSpace manager using sel4utils_bootstrap_vspace_with_bootinfo.\n"
                  "\tBe sure you've passed the correct component name for the new thread!\n");



/* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");

 /*
     * make a badged endpoint in the new process's cspace.  This copy
     * will be used to send an IPC to the original cap
     */

    /* TASK 4: make a cspacepath for the new endpoint cap */
    /* hint 1: vka_cspace_make_path()
     * void vka_cspace_make_path(vka_t *vka, seL4_CPtr slot, cspacepath_t *res)
     * @param vka Vka interface to use for allocation of objects.
     * @param slot A cslot allocated by the cspace alloc function
     * @param res Pointer to a cspacepath struct to fill out
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_4:
     *
     * hint 2: use the cslot of the endpoint allocated above
     */
    cspacepath_t ep_cap_path;
    seL4_CPtr new_ep_cap = 0;
    
    vka_cspace_make_path(&vka , ep_object.cptr , &ep_cap_path);

    /* TASK 5: copy the endpont cap and add a badge to the new cap */
    /* hint 1: sel4utils_mint_cap_to_process()
     * seL4_CPtr sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights rights, seL4_CapData_t data)
     * @param process Process to copy the cap to
     * @param src Path in the current cspace to copy the cap from
     * @param rights The rights of the new cap
     * @param data Extra data for the new cap (e.g., the badge)
     * @return 0 on failure, otherwise the slot in the processes cspace.
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_5:
     *
     * hint 2: for the rights, use seL4_AllRights
     * hint 3: for the badge use seL4_CapData_Badge_new()
     * seL4_CapData_t CONST seL4_CapData_Badge_new(seL4_Uint32 Badge)
     * @param[in] Badge The badge number to use
     * @return A CapData structure containing the desired badge info
     *
     * seL4_CapData_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_5:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 4: for the badge value use EP_BADGE
     */
//     seL4_CapData_t badge = seL4_CapData_Badge_new(EP_BADGE);
     new_ep_cap = sel4utils_mint_cap_to_process(&process , ep_cap_path , seL4_AllRights , EP_BADGE );

    ZF_LOGF_IF(new_ep_cap == 0, "Failed to mint a badged copy of the IPC endpoint into the new thread's CSpace.\n"
               "\tsel4utils_mint_cap_to_process takes a cspacepath_t: double check what you passed.\n");

    printf("NEW CAP SLOT: %" PRIxPTR ".\n", ep_cap_path.capPtr);

/* TASK 6: spawn the process */
    /* hint 1: sel4utils_spawn_process_v()
     * int sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc, char *argv[], int resume)
     * @param process Initialised sel4utils process struct.
     * @param vka Vka interface to use for allocation of frames.
     * @param vspace The current vspace.
     * @param argc The number of arguments.
     * @param argv A pointer to an array of strings in the current vspace.
     * @param resume 1 to start the process, 0 to leave suspended.
     * @return 0 on success, -1 on error.
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TASK_6:
     */

    seL4_Word argc = 1;
    char string_args[argc][WORD_STRING_SIZE];
    char* argv[argc];
    sel4utils_create_word_args(string_args, argv, argc, new_ep_cap);
    
    printf("Main : Start child \n");
    error = sel4utils_spawn_process_v(&process , &vka , &vspace , argc, (char**) &argv , 1);
    ZF_LOGF_IFERR(error, "Failed to spawn and start the new thread.\n"
                  "\tVerify: the new thread is being executed in the root thread's VSpace.\n"
                  "\tIn this case, the CSpaces are different, but the VSpaces are the same.\n"
                  "\tDouble check your vspace_t argument.\n");
    printf("Did start child \n");
    /* we are done, say hello */
    printf("main: hello world\n");


    while(1){}

    return 0;
}
