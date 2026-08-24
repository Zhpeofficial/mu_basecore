/** @file ArmMmuLibPageTablePool.c

  Pre-allocate page table memory from a reserved low-memory pool so that
  mapping large regions (e.g. the 6GB Upper DDR at 0x100000000) does not
  allocate page table pages from within the region being mapped. This avoids
  the chicken-and-egg fault where the page table write lands in the still
  unmapped Upper DDR.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#define PAGE_TABLE_POOL_PAGES  4096   // ~16 MB of page table memory
#define PAGE_TABLE_POOL_MAX_ADDR  0xA0000000ULL  // Keep the pool below the Upper DDR

STATIC VOID    *mPageTablePool[PAGE_TABLE_POOL_PAGES];
STATIC UINTN    mPageTablePoolCount = 0;
STATIC BOOLEAN  mPageTablePoolReady = FALSE;

/**
  Pre-allocate page table memory from low memory.

  Called from ArmMmuBaseLibConstructor during DXE initialization. Reserves
  low-memory pages (below the Upper DDR) so that later UpdateRegionMapping
  calls can allocate page tables without landing in the still-unmapped
  Upper DDR.
**/
EFI_STATUS
ArmMmuBaseLibPreAllocatePageTables (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  MaxAddress;
  EFI_STATUS            Status;
  UINTN                 i;

  if (mPageTablePoolReady) {
    return EFI_SUCCESS;
  }

  // Only pre-allocate when boot services are available (DXE). In SEC/PEI
  // gBS is not usable, so skip and fall back to AllocatePages later.
  if (gBS == NULL) {
    return EFI_UNSUPPORTED;
  }

  for (i = 0; i < PAGE_TABLE_POOL_PAGES; i++) {
    MaxAddress = PAGE_TABLE_POOL_MAX_ADDR;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    1,
                    &MaxAddress
                    );
    if (EFI_ERROR (Status)) {
      // No more low memory available; stop pre-allocating.
      break;
    }

    mPageTablePool[mPageTablePoolCount++] = (VOID *)(UINTN)MaxAddress;
  }

  mPageTablePoolReady = TRUE;
  DEBUG ((EFI_D_INFO, "Pre-allocated %u page table pages in low memory\n", mPageTablePoolCount));

  return mPageTablePoolCount > 0 ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

/**
  Allocates pages for the page table from the reserved pool.

  Falls back to AllocatePages if the pool is not available or exhausted
  (e.g. in SEC/PEI or for very large requests).

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
