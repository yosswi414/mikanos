#include "memory_manager.hpp"
#include "logger.hpp"

BitmapMemoryManager::BitmapMemoryManager() : alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}} {
    // Log(kWarn, "alloc_map_ size: %d\n", alloc_map_.size());
    // for (int i = 0; i < alloc_map_.size(); ++i){
    //     alloc_map_[i] = 0;
    // }
}

void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames) {
    for (size_t i = 0; i < num_frames; ++i) SetBit(FrameID{start_frame.ID() + i}, true);
}

void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end){
    range_begin_ = range_begin;
    range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const {
    auto line_index = frame.ID() / kBitsPerMapLine;
    auto bit_index = frame.ID() % kBitsPerMapLine;
    // Log(kInfo, "GetBit(): line/bit: %lu/%lu, alloc_map_[line] = %lx\n", line_index, bit_index, alloc_map_[line_index]);
    return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated) {
    auto line_index = frame.ID() / kBitsPerMapLine;
    auto bit_index = frame.ID() % kBitsPerMapLine;

    const auto mask = static_cast<MapLineType>(1) << bit_index;
    if (allocated) {
        alloc_map_[line_index] |= mask;
    } else {
        alloc_map_[line_index] &= ~mask;
    }
}

// first-fit
WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames) {
    size_t start_frame_id = range_begin_.ID();
    // Log(kWarn, "[Allocate()] memory manager range: %llu - %llu\n", range_begin_.ID(), range_end_.ID());
    // Log(kWarn, "[Allocate()] num_frames = %llu\n", num_frames);
    while (true) {
        size_t i = 0;
        for (; i < num_frames; ++i){
            if (start_frame_id + i >= range_end_.ID()) return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
            // break if already allocated
            if (GetBit(FrameID{start_frame_id + i})) {
                // Log(kInfo, "[Allocate()] failed to allocate. start_frame_id = %d, i = %d\n", start_frame_id, i);
                break;
            }
        }
        if(i == num_frames) {
            // found the first (num_frames) frames space
            MarkAllocated(FrameID{start_frame_id}, num_frames);
            return {FrameID{start_frame_id}, MAKE_ERROR(Error::kSuccess)};
        }
        // re-search from the next frame
        start_frame_id += i + 1;
    }
}

Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames){
    for (size_t i = 0; i < num_frames; ++i) SetBit(FrameID{start_frame.ID() + i}, false);
    return MAKE_ERROR(Error::kSuccess);
}

extern "C" caddr_t program_break, program_break_end;

Error InitializeHeap(BitmapMemoryManager& memory_manager){
    const int kHeapFrames = 64 * 512;   // 64 * 512 [frames] * 4 [KiB / frame] = 128 [MiB]
    const auto heap_start = memory_manager.Allocate(kHeapFrames);

    // Log(kWarn, "Allocate() end\n");

    if (heap_start.error) return heap_start.error;

    // Log(kWarn, "allocated successful\n");

    program_break = reinterpret_cast<caddr_t>(heap_start.value.ID() * kBytesPerFrame);
    program_break_end = program_break + kHeapFrames * kBytesPerFrame;
    return MAKE_ERROR(Error::kSuccess);
}