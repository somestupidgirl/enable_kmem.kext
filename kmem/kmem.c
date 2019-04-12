#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <miscfs/devfs/devfs.h>

#include "kernel_resolver.h"

kern_return_t kmem_start(kmod_info_t *ki, void *d);
kern_return_t kmem_stop(kmod_info_t *ki, void *d);

static void *dev_kmem_devnode = NULL; // devfs node for /dev/kmem

#define RESOLVE_KSYM_OR_DIE(VARIABLE, SYMBOL) do {  \
    if ((VARIABLE = (void*)find_kernel_symbol(SYMBOL)) == NULL) { \
printf("[kmem.kext][-] Couldn't resolve required symbol `" #SYMBOL "'\n"); \
        return KERN_FAILURE; \
    }} while(0)

/* _doprnt_hide_pointers */
static boolean_t *dev_kmem_mask_top_bit = NULL;
static boolean_t *dev_kmem_enabled = NULL;

static inline uintptr_t get_cr0(void)
{
    uintptr_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
    return(cr0);
}

static inline void set_cr0(uintptr_t value)
{
    __asm__ volatile("mov %0, %%cr0" : : "r" (value));
}

kern_return_t kmem_start(kmod_info_t *ki, void *d)
{
    RESOLVE_KSYM_OR_DIE(dev_kmem_mask_top_bit, "_dev_kmem_mask_top_bit");
    RESOLVE_KSYM_OR_DIE(dev_kmem_enabled, "_dev_kmem_enabled");

    /* Need this to for lseek() to work properly. 0x7fffff8000000000 is then used
     * as the kernel base instead (MSB unset). The kmem device will patch the address
     * in the module itself.
     */
    *dev_kmem_mask_top_bit = TRUE;
    if (*dev_kmem_enabled) {
        printf("[kmem.kext][+] dev_kmem_enabled is set, using existing /dev/kmem device.\n");
        goto dev_kmem_exists;
    }

    if ((dev_kmem_devnode = devfs_make_node(makedev(3, 1), DEVFS_CHAR,
                                            UID_ROOT, GID_KMEM, 0640, "kmem")))
    {
        printf("[kmem.kext][+] Loaded /dev/kmem device.\n");
        *dev_kmem_enabled = TRUE;
    } else {
        printf("[kmem.kext][-] Failed to load /dev/kmem device.\n");
        return KERN_FAILURE;
    }
    
dev_kmem_exists:

#define CR0_WP 0x00010000
    set_cr0(get_cr0() & ~CR0_WP);
    return KERN_SUCCESS;
}

kern_return_t kmem_stop(kmod_info_t *ki, void *d)
{
    if (dev_kmem_devnode) {
        devfs_remove(dev_kmem_devnode);
        printf("[kmem.kext][+] Unloaded /dev/kmem device.\n");
    }

    return KERN_SUCCESS;
}
