
#include "paging.hpp"

#include <array>

#include "asmfunc.h"

// See: https://wiki.osdev.org/Page_Tables#2_MiB_pages_2
namespace {
    const uint64_t kPageSize4K = 4096;               // 2^12
    const uint64_t kPageSize2M = 512 * kPageSize4K;  // 2^21
    const uint64_t kPageSize1G = 512 * kPageSize2M;  // 2^30

    // const uint16_t kPageFlagP = 1 << 0;    // Present
    // const uint16_t kPageFlagRW = 1 << 1;   // Read / Write
    // const uint16_t kPageFlagUS = 1 << 2;   // User / Supervisor
    // const uint16_t kPageFlagPWT = 1 << 3;  // Write-Through
    // const uint16_t kPageFlagPCD = 1 << 4;  // Cache Disable
    // const uint16_t kPageFlagA = 1 << 5;    // Accessed
    // const uint16_t kPageFlagAVL = 1 << 6;  // AVaiLable
    // const uint16_t kPageFlagPS = 1 << 7;   // Page Size (0: 1 GiB, 1: 2 MiB)

    // 4 level paging
    // Page Map Level 4 table
    alignas(kPageSize4K) std::array<uint64_t, 512> pml4_table;
    // Page Directory Pointer table
    alignas(kPageSize4K) std::array<uint64_t, 512> pdp_table;
    // Page Directory
    alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory;
}  // namespace

void SetupIdentityPageTable() {
    pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
    for (int i_pdpt = 0; i_pdpt < page_directory.size(); ++i_pdpt) {
        pdp_table[i_pdpt] = reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
        for (int i_pd = 0; i_pd < 512; ++i_pd) {
            page_directory[i_pdpt][i_pd] = i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
        }
    }
    SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}