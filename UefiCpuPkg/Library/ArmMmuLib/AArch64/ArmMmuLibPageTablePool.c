/** @file ArmMmuLibPageTablePool.c

  Pre-allocate page table memory so that mapping large regions (e.g. the 6GB
  Upper DDR at 0x100000000) does not allocate page table pages from within
  the region being mapped. This avoids the chicken-and-egg fault where the
  page table write lands in the still-unmapped Upper DDR.

  AllocateAnyPages is used (no gBS dependency) so this library stays usable
  in SEC/PEI/DXE. During DXE initialization the Upper DDR is not yet in the
  GCD, so AllocateAnyPages naturally picks low (already-mapped) memory.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#define PAGE_TABLE_POOL_PAGES  4096   // ~16 MB of page table memory

STATIC VOID    *mPageTablePool[PAGE_TABLE_POOL_PAGES];
STATIC UINTN    mPageTablePoolCount = 0;
STATIC BOOLEAN  mPageTablePoolReady = FALSE;

/**
  Pre-allocate page table memory.

  Called from ArmMmuBaseLibConstructor. At DXE init the Upper DDR is not yet
  in the GCD, so AllocateAnyPages returns low (already-mapped) memory.
**/
EFI_STATUS
ArmMmuBaseLibPreAllocatePageTables (
  VOID
  )
{
  VOID                *Page;
  UINTN               i;

  if (mPageTablePoolReady) {
    return EFI_SUCCESS;
  }

  for (i = 0; i < PAGE_TABLE_POOL_PAGES; i++) {
    Page = AllocatePages (1);
    if (Page == NULL) {
      // No more memory available; stop pre-allocating.
      break;
    }

    mPageTablePool[mPageTablePoolCount++] = Page;
  }

  mPageTablePoolReady = TRUE;
  DEBUG ((EFI_D_INFO, "Pre-allocated %u page table pages in low memory\n", mPageTablePoolCount));

  return mPageTablePoolCount > 0 ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

/**
  Allocates pages for the page table from the reserved pool.

  Falls back to AllocatePages if the pool is not available or exhausted.

  @param[in]  Pages  The number of pages to allocate

  @return A pointer to the allocated buffer or NULL if allocation fails
**/
VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  )
{
  if (Pages == 1 && mPageTablePoolCount > 0) {
    return mPageTablePool[--mPageTablePoolCount];
  }

  return AllocatePages (Pages);
}
