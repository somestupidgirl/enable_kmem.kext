//
//  kernel_resolver.h
//  KMem
//
//  Created by Josh de Kock on 08/04/2019.
//  Copyright © 2019 Itanimulli. All rights reserved.
//

#ifndef kernel_resolver_h
#define kernel_resolver_h

#include <mach/mach_types.h>
#include <mach-o/loader.h>

#define KERNEL_BASE 0xffffff8000200000

void *find_symbol(struct mach_header_64 *mh, const char *name);
void *find_kernel_symbol(const char *symbol);

#endif /* kernel_resolver_h */
