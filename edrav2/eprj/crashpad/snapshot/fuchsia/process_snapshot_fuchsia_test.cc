// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "snapshot/fuchsia/memory_map_region_snapshot_fuchsia.h"

#include <dbghelp.h>
#include <zircon/syscalls.h>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "gtest/gtest.h"
#include "snapshot/fuchsia/process_snapshot_fuchsia.h"
#include "test/multiprocess_exec.h"
#include "util/fuchsia/scoped_task_suspend.h"

namespace crashpad {
namespace test {
namespace {

constexpr struct {
  uint32_t zircon_perm;
  size_t pages;
  uint32_t minidump_perm;
} kTestMappingPermAndSizes[] = {
    // Zircon doesn't currently allow write-only, execute-only, or
    // write-execute-only, returning ZX_ERR_INVALID_ARGS on map.
    {0, 5, PAGE_NOACCESS},
    {ZX_VM_PERM_READ, 6, PAGE_READONLY},
    // {ZX_VM_PERM_WRITE, 7, PAGE_WRITECOPY},
    // {ZX_VM_PERM_EXECUTE, 8, PAGE_EXECUTE},
    {ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, 9, PAGE_READWRITE},
    {ZX_VM_PERM_READ | ZX_VM_PERM_EXECUTE, 10, PAGE_EXECUTE_READ},
    // {ZX_VM_PERM_WRITE | ZX_VM_PERM_EXECUTE, 11, PAGE_EXECUTE_WRITECOPY},
    {ZX_VM_PERM_READ | ZX_VM_PERM_WRITE | ZX_VM_PERM_EXECUTE,
     12,
     PAGE_EXECUTE_READWRITE},
};

CRASHPAD_CHILD_TEST_MAIN(AddressSpaceChildTestMain) {
  // Create specifically sized mappings/permissions and write the address in
  // our address space to the parent so that the reader can check they're read
  // correctly.
  for (const auto& t : kTestMappingPermAndSizes) {
    zx_handle_t vmo = ZX_HANDLE_INVALID;
    const size_t size = t.pages * PAGE_SIZE;
    zx_status_t status = zx_vmo_create(size, 0, &vmo);
    ZX_CHECK(status == ZX_OK, status) << "zx_vmo_create";
    uintptr_t mapping_addr = 0;
    status = zx_vmar_map(
        zx_vmar_root_self(), t.zircon_perm, 0, vmo, 0, size, &mapping_addr);
    ZX_CHECK(status == ZX_OK, status) << "zx_vmar_map";
    CheckedWriteFile(StdioFileHandle(StdioStream::kStandardOutput),
                     &mapping_addr,
                     sizeof(mapping_addr));
  }

  CheckedReadFileAtEOF(StdioFileHandle(StdioStream::kStandardInput));
  return 0;
}

bool HasSingleMatchingMapping(
    const std::vector<const MemoryMapRegionSnapshot*>& memory_map,
    uintptr_t address,
    size_t size,
    uint32_t perm) {
  const MemoryMapRegionSnapshot* matching = nullptr;
  for (const auto* region : memory_map) {
    const MINIDUMP_MEMORY_INFO& mmi = region->AsMinidumpMemoryInfo();
    if (mmi.BaseAddress == address) {
      if (matching) {
        LOG(ERROR) << "multiple mappings matching address";
        return false;
      }
      matching = region;
    }
  }

  if (!matching)
    return false;

  const MINIDUMP_MEMORY_INFO& matching_mmi = matching->AsMinidumpMemoryInfo();
  return matching_mmi.Protect == perm && matching_mmi.RegionSize == size;
}

class AddressSpaceTest : public MultiprocessExec {
 public:
  AddressSpaceTest() : MultiprocessExec() {
    SetChildTestMainFunction("AddressSpaceChildTestMain");
  }
  ~AddressSpaceTest() {}

 private:
  void MultiprocessParent() override {
    uintptr_t test_addresses[arraysize(kTestMappingPermAndSizes)];
    for (size_t i = 0; i < arraysize(test_addresses); ++i) {
      ASSERT_TRUE(ReadFileExactly(
          ReadPipeHandle(), &test_addresses[i], sizeof(test_addresses[i])));
    }

    ScopedTaskSuspend suspend(*ChildProcess());

    ProcessSnapshotFuchsia process_snapshot;
    ASSERT_TRUE(process_snapshot.Initialize(*ChildProcess()));

    for (size_t i = 0; i < arraysize(test_addresses); ++i) {
      const auto& t = kTestMappingPermAndSizes[i];
      EXPECT_TRUE(HasSingleMatchingMapping(process_snapshot.MemoryMap(),
                                           test_addresses[i],
                                           t.pages * PAGE_SIZE,
                                           t.minidump_perm))
          << base::StringPrintf(
                 "index %zu, zircon_perm 0x%x, minidump_perm 0x%x",
                 i,
                 t.zircon_perm,
                 t.minidump_perm);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(AddressSpaceTest);
};

TEST(ProcessSnapshotFuchsiaTest, AddressSpaceMapping) {
  AddressSpaceTest test;
  test.Run();
}

}  // namespace
}  // namespace test
}  // namespace crashpad
