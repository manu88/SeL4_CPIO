#include <stdio.h>
#include <stdbool.h>
#include <cpio/cpio.h>

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char _cpio_archive[];


#define APP_NAME "app"

int main(void)
{
    printf("init started\n");

    /* parse the cpio image */
    printf( "\nStarting \"%s\"...\n", APP_NAME);
    unsigned long elf_size;
    char* elf_base = cpio_get_file(_cpio_archive, APP_NAME, &elf_size);
    if (elf_base == NULL) {
        printf("Unable to locate cpio header for %s", APP_NAME);
        return 1;
    }

    printf("CPIO Size : %lu\n", elf_size);

    return 0;
}
