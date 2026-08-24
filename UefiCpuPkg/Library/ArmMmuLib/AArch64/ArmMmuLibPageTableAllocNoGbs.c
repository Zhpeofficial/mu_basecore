#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>

VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
}
