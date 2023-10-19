#pragma once

#include <cstddef>

/**
 * @brief 静的に確保するページディレクトリの個数
 *
 * SetupIdentityPageMap で使用される
 * kPageDirectoryCount * 1 GiB の仮想アドレスがマッピングされる
 * kPDC [page directories] * 512 [page tables / page directory] * 2 [MiB / page table] = kPDC [GiB]
 */
const size_t kPageDirectoryCount = 64;

/**
 * @brief 仮想アドレス = 物理アドレスとなるようにページテーブルを設定
 * 最終的に CR3 レジスタが正しく設定されたページテーブルを指すようになる
 */
void SetupIdentityPageTable();

void InitializePaging();
