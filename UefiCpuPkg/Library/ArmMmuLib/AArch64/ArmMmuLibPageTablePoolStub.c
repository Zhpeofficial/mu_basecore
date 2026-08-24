/** @file ArmMmuLibPageTablePoolStub.c

  No-op page table pool for SEC/PEI. The Upper DDR mapping (which needs the
  pre-allocated pool) only happens in DXE via ArmMmuBaseLib. Here we simply
  fall back to AllocatePages.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>

/**
  Allocates pages for the page table.

  @param[in]  Pages  The number of pages to allocate

  @return A pointer to the allocated buffer or NULL if allocation fails
**/
VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
}

/**
  No-op for SEC/PEI: the page table pool is only set up in DXE.
**/
EFI_STATUS
EFIAPI
ArmMmuBaseLibPreAllocatePageTables (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
