//
//  kernel_resolver.h
//  KMem
//
//  Created by Josh de Kock on 08/04/2019.
//  Copyright © 2019 Itanimulli. All rights reserved.
//

#include "kernel_resolver.h"

#include <mach-o/nlist.h>
#include <sys/systm.h>
#include <sys/types.h>

vm_offset_t *vm_kernel_slide;

static struct segment_command_64 *find_segment_64(struct mach_header_64 *mh, const char *segname)
{
    struct load_command *lc;
    struct segment_command_64 *seg, *foundseg = NULL;
    
    /* first load command begins straight after the mach header */
    lc = (struct load_command *)((uint64_t)mh + sizeof(struct mach_header_64));
    while ((uint64_t)lc < (uint64_t)mh + (uint64_t)mh->sizeofcmds) {
        if (lc->cmd == LC_SEGMENT_64) {
            /* evaluate segment */
            seg = (struct segment_command_64 *)lc;
            if (strcmp(seg->segname, segname) == 0) {
                foundseg = seg;
                break;
            }
        }
        
        /* next load command */
        lc = (struct load_command *)((uint64_t)lc + (uint64_t)lc->cmdsize);
    }
    
    return foundseg;
}

static struct load_command *find_load_command(struct mach_header_64 *mh, uint32_t cmd)
{
    struct load_command *lc, *foundlc = NULL;
    
    /* first load command begins straight after the mach header */
    lc = (struct load_command *)((uint64_t)mh + sizeof(struct mach_header_64));
    while ((uint64_t)lc < (uint64_t)mh + (uint64_t)mh->sizeofcmds) {
        if (lc->cmd == cmd) {
            foundlc = (struct load_command *)lc;
            break;
        }
        
        /* next load command*/
        lc = (struct load_command *)((uint64_t)lc + (uint64_t)lc->cmdsize);
    }
    
    return foundlc;
}

void *find_symbol(struct mach_header_64 *mh, const char *name)
{
    struct symtab_command *lc_symtab = NULL;
    struct segment_command_64 *linkedit = NULL;
    struct nlist_64 *symtab = NULL;
    char *strtab = NULL;
    void *addr = NULL;

    /* Check for Mach Header. We should probably also check for MH_CIGAM_64 but
     * it's probably ok to assume that byte-order is the same as host OS. */
    if (mh->magic != MH_MAGIC_64) {
        printf("%s: magic number doesn't match - 0x%x\n", __func__, mh->magic);
        return NULL;
    }
    
    /* Find the __LINKEDIT segment and LC_SYMTAB command */
    linkedit = find_segment_64(mh, SEG_LINKEDIT);
    if (!linkedit) {
        printf("%s: couldn't find __LINKEDIT\n", __func__);
        return NULL;
    }
    
    lc_symtab = (struct symtab_command *)find_load_command(mh, LC_SYMTAB);
    if (!lc_symtab) {
        printf("%s: couldn't find LC_SYMTAB\n", __func__);
        return NULL;
    }
    
    strtab = (void *)((int64_t)(linkedit->vmaddr - linkedit->fileoff) + lc_symtab->stroff);
    symtab = (struct nlist_64 *)((int64_t)(linkedit->vmaddr - linkedit->fileoff) + lc_symtab->symoff);

     for (uint64_t i = 0; i < lc_symtab->nsyms; i++) {
        char *str = (char *)strtab + symtab[i].n_un.n_strx;
 
        if (strcmp(str, name) == 0) {
            addr = (void *)symtab[i].n_value;
        }
    }
    
    return addr;
}

void *find_kernel_symbol(const char *symbol)
{
    static vm_offset_t kernel_slide = 0;

#if !defined(NO_KALSR)
    if (!kernel_slide) {
        vm_offset_t slide_address = 0;
        /* Can use other symbols than printf in KPI. See xnu/osfmk/vm/vm_kern.c */
        vm_kernel_unslide_or_perm_external((vm_offset_t)printf, &slide_address);
        kernel_slide = (vm_offset_t)printf - slide_address;
    }
#endif

    return find_symbol((struct mach_header_64 *)(kernel_slide + KERNEL_BASE), symbol);
}
