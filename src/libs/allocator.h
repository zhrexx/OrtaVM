#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef unsigned long uintptr_t;
typedef size_t Marker;

#define ARENA_CAPACITY (1024 * 1024 * 4)
#define BLOCK_ALIGN 16
#define GUARD_SIZE 8
#define CANARY_VALUE 0xDEADC0DE

typedef struct {
    size_t total_allocated;
    size_t current_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    size_t deallocation_count;
    size_t failed_allocations;
} ArenaStats;

typedef struct {
    char buffer[ARENA_CAPACITY];
    size_t size;
    ArenaStats stats;
} AArena;

typedef struct {
    size_t size;
    uint32_t canary;
} AllocationHeader;

static AArena global_arena = {0};

void *aarena_alloc(AArena *arena, size_t size) {
    if (arena->size + size + sizeof(AllocationHeader) + GUARD_SIZE > ARENA_CAPACITY) {
        arena->stats.failed_allocations++;
        return NULL;
    }

    size_t aligned_size = (size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    uintptr_t current = (uintptr_t)&arena->buffer[arena->size];
    uintptr_t aligned = (current + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    size_t padding = aligned - current;
    
    arena->size += padding;
    AllocationHeader *header = (AllocationHeader *)&arena->buffer[arena->size];
    header->size = size;
    header->canary = CANARY_VALUE;
    
    void *user_ptr = (void *)(header + 1);
    
    *(uint64_t *)((char *)user_ptr + aligned_size) = 0xCCCCCCCCCCCCCCCC;
    
    arena->size += sizeof(AllocationHeader) + aligned_size + GUARD_SIZE;
    
    arena->stats.total_allocated += size;
    arena->stats.current_allocated += size;
    arena->stats.allocation_count++;
    
    if (arena->stats.current_allocated > arena->stats.peak_allocated) {
        arena->stats.peak_allocated = arena->stats.current_allocated;
    }
    
    return user_ptr;
}

int aarena_check_memory(void *ptr) {
    if (!ptr) return 0;
    
    AllocationHeader *header = ((AllocationHeader *)ptr) - 1;
    size_t aligned_size = (header->size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    
    if (header->canary != CANARY_VALUE) {
        printf("Memory corruption: Canary value\n");
        return 0;
    }
    
    if (*(uint64_t *)((char *)ptr + aligned_size) != 0xCCCCCCCCCCCCCCCC) {
        printf("Memory corruption: Guard value\n");
        return 0;
    }
    
    return 1;
}

size_t aarena_alloc_size(void *ptr) {
    if (!ptr) return 0;
    AllocationHeader *header = ((AllocationHeader *)ptr) - 1;
    return header->size;
}

void *aarena_alloc_aligned(AArena *arena, size_t size, size_t alignment) {
    uintptr_t current = (uintptr_t)&arena->buffer[arena->size];
    uintptr_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
    size_t padding = aligned - current;
    if (arena->size + size + padding > ARENA_CAPACITY) {
        arena->stats.failed_allocations++;
        return NULL;
    }
    arena->size += padding;
    return aarena_alloc(arena, size);
}

void *aarena_calloc(AArena *arena, size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = aarena_alloc(arena, total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *aarena_realloc(AArena *arena, void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) return aarena_alloc(arena, new_size);
    
    AllocationHeader *header = ((AllocationHeader *)ptr) - 1;
    size_t aligned_old = (old_size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    size_t aligned_new = (new_size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    
    if ((char *)ptr + aligned_old + GUARD_SIZE == arena->buffer + arena->size - GUARD_SIZE) {
        if (aligned_new > aligned_old) {
            size_t diff = aligned_new - aligned_old;
            if (arena->size + diff > ARENA_CAPACITY) {
                arena->stats.failed_allocations++;
                return NULL;
            }
            arena->size += diff;
            header->size = new_size;
            *(uint64_t *)((char *)ptr + aligned_new) = 0xCCCCCCCCCCCCCCCC;
            
            arena->stats.total_allocated += (new_size - old_size);
            arena->stats.current_allocated += (new_size - old_size);
            
            if (arena->stats.current_allocated > arena->stats.peak_allocated) {
                arena->stats.peak_allocated = arena->stats.current_allocated;
            }
        } else {
            arena->size -= (aligned_old - aligned_new);
            header->size = new_size;
            *(uint64_t *)((char *)ptr + aligned_new) = 0xCCCCCCCCCCCCCCCC;
            
            arena->stats.current_allocated -= (old_size - new_size);
        }
        return ptr;
    } else {
        void *new_ptr = aarena_alloc(arena, new_size);
        if (!new_ptr) return NULL;
        memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
        
        return new_ptr;
    }
}

void aarena_reset(AArena *arena) {
    arena->size = 0;
    arena->stats.current_allocated = 0;
    arena->stats.deallocation_count++;
}

void aarena_free_to(AArena *arena, Marker marker) {
    if (marker <= arena->size) {
        size_t freed_size = arena->size - marker;
        arena->size = marker;
        arena->stats.current_allocated -= freed_size;
        arena->stats.deallocation_count++;
    }
}

Marker aarena_marker(AArena *arena) {
    return arena->size;
}

size_t aarena_remaining(AArena *arena) {
    return ARENA_CAPACITY - arena->size;
}

int aarena_contains(AArena *arena, void *ptr) {
    return (ptr >= (void*)arena->buffer) && (ptr < (void*)(arena->buffer + ARENA_CAPACITY));
}

char *aarena_strdup(AArena *arena, const char *str) {
    size_t len = strlen(str) + 1;
    char *dup = aarena_alloc(arena, len);
    if (dup) memcpy(dup, str, len);
    return dup;
}

void *aarena_memdup(AArena *arena, const void *src, size_t size) {
    void *dest = aarena_alloc(arena, size);
    if (dest) memcpy(dest, src, size);
    return dest;
}

void aarena_dump(AArena *arena) {
    printf("Arena: %zu used, %zu free\n", arena->size, aarena_remaining(arena));
    printf("Stats: Total allocated: %zu bytes\n", arena->stats.total_allocated);
    printf("       Current allocated: %zu bytes\n", arena->stats.current_allocated);
    printf("       Peak allocated: %zu bytes\n", arena->stats.peak_allocated);
    printf("       Allocation count: %zu\n", arena->stats.allocation_count);
    printf("       Deallocation count: %zu\n", arena->stats.deallocation_count);
    printf("       Failed allocations: %zu\n", arena->stats.failed_allocations);
    printf("       Utilization: %.2f%%\n", (float)arena->size / ARENA_CAPACITY * 100.0f);
    printf("       Effective utilization: %.2f%%\n", (float)arena->stats.current_allocated / arena->size * 100.0f);
}

void aarena_reset_stats(AArena *arena) {
    arena->stats.total_allocated = arena->stats.current_allocated;
    arena->stats.peak_allocated = arena->stats.current_allocated;
    arena->stats.allocation_count = 0;
    arena->stats.deallocation_count = 0;
    arena->stats.failed_allocations = 0;
}

typedef struct {
    ArenaStats stats;
    char *start;
    size_t size;
    size_t offset;
} ARegion;

ARegion create_region(AArena *arena, size_t region_size) {
    ARegion region = {0};
    region_size = (region_size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    if (arena->size + region_size > ARENA_CAPACITY) {
        arena->stats.failed_allocations++;
        return region;
    }
    
    region.start = &arena->buffer[arena->size];
    region.size = region_size;
    region.offset = 0;
    arena->size += region_size;
    return region;
}

void *region_alloc(ARegion *region, size_t size) {
    size_t aligned_size = (size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    if (region->offset + aligned_size > region->size) {
        region->stats.failed_allocations++;
        return NULL;
    }
    
    void *ptr = region->start + region->offset;
    region->offset += aligned_size;
    
    region->stats.total_allocated += size;
    region->stats.current_allocated += size;
    region->stats.allocation_count++;
    
    if (region->stats.current_allocated > region->stats.peak_allocated) {
        region->stats.peak_allocated = region->stats.current_allocated;
    }
    
    return ptr;
}

void *region_alloc_aligned(ARegion *region, size_t size, size_t alignment) {
    uintptr_t current = (uintptr_t)(region->start + region->offset);
    uintptr_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
    size_t padding = aligned - current;
    
    size_t aligned_size = (size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    if (region->offset + aligned_size + padding > region->size) {
        region->stats.failed_allocations++;
        return NULL;
    }
    
    region->offset += padding;
    return region_alloc(region, size);
}

void *region_calloc(ARegion *region, size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = region_alloc(region, total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void region_reset(ARegion *region) {
    region->offset = 0;
    region->stats.current_allocated = 0;
    region->stats.deallocation_count++;
}

void region_free_to(ARegion *region, Marker marker) {
    if (marker <= region->offset) {
        size_t freed_size = region->offset - marker;
        region->offset = marker;
        region->stats.current_allocated -= freed_size;
        region->stats.deallocation_count++;
    }
}

Marker region_marker(ARegion *region) {
    return region->offset;
}

size_t region_remaining(ARegion *region) {
    return region->size - region->offset;
}

int region_contains(ARegion *region, void *ptr) {
    return (ptr >= (void*)region->start) && (ptr < (void*)(region->start + region->size));
}

char *region_strdup(ARegion *region, const char *str) {
    size_t len = strlen(str) + 1;
    char *dup = region_alloc(region, len);
    if (dup) memcpy(dup, str, len);
    return dup;
}

void *region_memdup(ARegion *region, const void *src, size_t size) {
    void *dest = region_alloc(region, size);
    if (dest) memcpy(dest, src, size);
    return dest;
}

ARegion create_subregion(ARegion *parent, size_t subregion_size) {
    ARegion sub = {0};
    subregion_size = (subregion_size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    if (parent->offset + subregion_size > parent->size) {
        parent->stats.failed_allocations++;
        return sub;
    }
    
    sub.start = parent->start + parent->offset;
    sub.size = subregion_size;
    sub.offset = 0;
    parent->offset += subregion_size;
    return sub;
}

void region_dump(ARegion *region) {
    printf("Region: %zu/%zu used, %zu free\n", 
           region->offset, region->size, region_remaining(region));
    printf("Stats: Total allocated: %zu bytes\n", region->stats.total_allocated);
    printf("       Current allocated: %zu bytes\n", region->stats.current_allocated);
    printf("       Peak allocated: %zu bytes\n", region->stats.peak_allocated);
    printf("       Allocation count: %zu\n", region->stats.allocation_count);
    printf("       Deallocation count: %zu\n", region->stats.deallocation_count);
    printf("       Failed allocations: %zu\n", region->stats.failed_allocations);
    printf("       Utilization: %.2f%%\n", (float)region->offset / region->size * 100.0f);
    printf("       Effective utilization: %.2f%%\n", 
           region->offset > 0 ? (float)region->stats.current_allocated / region->offset * 100.0f : 0.0f);
}

void region_reset_stats(ARegion *region) {
    region->stats.total_allocated = region->stats.current_allocated;
    region->stats.peak_allocated = region->stats.current_allocated;
    region->stats.allocation_count = 0;
    region->stats.deallocation_count = 0;
    region->stats.failed_allocations = 0;
}

void *aarena_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

void *aarena_memset(void *dest, int val, size_t n) {
    return memset(dest, val, n);
}

int aarena_memcmp(const void *s1, const void *s2, size_t n) {
    return memcmp(s1, s2, n);
}

char *arena_slurp_file(AArena *arena, const char *filename, size_t *size_out) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        arena->stats.failed_allocations++;
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = aarena_alloc(arena, file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        return NULL;
    }
    
    buffer[file_size] = '\0';
    if (size_out) {
        *size_out = file_size;
    }
    return buffer;
}

typedef struct PoolBlock {
    struct PoolBlock *next;
} PoolBlock;

typedef struct {
    ArenaStats stats;
    PoolBlock *free_list;
    char *memory;
    size_t block_size;
    size_t capacity;
    size_t used_blocks;
    size_t total_blocks;
} APool;

APool *apool_create(AArena *arena, size_t block_size, size_t num_blocks) {
    block_size = block_size < sizeof(PoolBlock) ? sizeof(PoolBlock) : block_size;
    block_size = (block_size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    
    size_t pool_mem_size = block_size * num_blocks;
    size_t total_size = sizeof(APool) + pool_mem_size;
    
    APool *pool = aarena_alloc(arena, total_size);
    if (!pool) return NULL;
    
    memset(pool, 0, sizeof(APool));
    
    pool->block_size = block_size;
    pool->capacity = pool_mem_size;
    pool->total_blocks = num_blocks;
    pool->memory = (char *)(pool + 1);
    
    pool->free_list = NULL;
    for (size_t i = 0; i < num_blocks; i++) {
        PoolBlock *block = (PoolBlock *)(pool->memory + (i * block_size));
        block->next = pool->free_list;
        pool->free_list = block;
    }
    
    return pool;
}

void *apool_alloc(APool *pool) {
    if (!pool->free_list) {
        pool->stats.failed_allocations++;
        return NULL;
    }
    
    PoolBlock *block = pool->free_list;
    pool->free_list = block->next;
    pool->used_blocks++;
    
    pool->stats.total_allocated += pool->block_size;
    pool->stats.current_allocated += pool->block_size;
    pool->stats.allocation_count++;
    
    if (pool->stats.current_allocated > pool->stats.peak_allocated) {
        pool->stats.peak_allocated = pool->stats.current_allocated;
    }
    
    memset(block, 0, pool->block_size);
    
    return block;
}

void apool_free(APool *pool, void *ptr) {
    if (!ptr) return;
    
    if (ptr < (void*)pool->memory || ptr >= (void*)(pool->memory + pool->capacity)) {
        printf("Pool free error: Pointer out of range\n");
        return;
    }
    
    uintptr_t block_addr = (uintptr_t)ptr;
    uintptr_t base_addr = (uintptr_t)pool->memory;
    if ((block_addr - base_addr) % pool->block_size != 0) {
        printf("Pool free error: Misaligned pointer\n");
        return;
    }
    
    PoolBlock *block = (PoolBlock*)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
    pool->used_blocks--;
    
    pool->stats.current_allocated -= pool->block_size;
    pool->stats.deallocation_count++;
}

void apool_reset(APool *pool) {
    if (!pool) return;
    
    pool->free_list = NULL;
    for (size_t i = 0; i < pool->total_blocks; i++) {
        PoolBlock *block = (PoolBlock *)(pool->memory + (i * pool->block_size));
        block->next = pool->free_list;
        pool->free_list = block;
    }
    
    pool->used_blocks = 0;
    pool->stats.current_allocated = 0;
    pool->stats.deallocation_count++;
}

int apool_contains(APool *pool, void *ptr) {
    return (ptr >= (void*)pool->memory) && 
           (ptr < (void*)(pool->memory + pool->capacity)) && 
           (((uintptr_t)ptr - (uintptr_t)pool->memory) % pool->block_size == 0);
}

size_t apool_block_size(APool *pool) {
    return pool->block_size;
}

size_t apool_available_blocks(APool *pool) {
    return pool->total_blocks - pool->used_blocks;
}

float apool_utilization(APool *pool) {
    return (float)pool->used_blocks / pool->total_blocks * 100.0f;
}

void apool_dump(APool *pool) {
    printf("Pool: %zu/%zu blocks used (%.2f%%)\n", 
           pool->used_blocks, pool->total_blocks, apool_utilization(pool));
    printf("Block size: %zu bytes\n", pool->block_size);
    printf("Stats: Total allocated: %zu bytes\n", pool->stats.total_allocated);
    printf("       Current allocated: %zu bytes\n", pool->stats.current_allocated);
    printf("       Peak allocated: %zu bytes\n", pool->stats.peak_allocated);
    printf("       Allocation count: %zu\n", pool->stats.allocation_count);
    printf("       Deallocation count: %zu\n", pool->stats.deallocation_count);
    printf("       Failed allocations: %zu\n", pool->stats.failed_allocations);
}

void apool_reset_stats(APool *pool) {
    pool->stats.total_allocated = pool->stats.current_allocated;
    pool->stats.peak_allocated = pool->stats.current_allocated;
    pool->stats.allocation_count = 0;
    pool->stats.deallocation_count = 0;
    pool->stats.failed_allocations = 0;
}

void *get_next_ptr(void *ptr) {
    if (!ptr) return NULL;
    
    AllocationHeader *header = ((AllocationHeader *)ptr) - 1;
    size_t size = header->size;
    size_t aligned_size = (size + (BLOCK_ALIGN-1)) & ~(BLOCK_ALIGN-1);
    
    char *next_header_addr = (char *)ptr + aligned_size + GUARD_SIZE;
    
    return (void *)(((AllocationHeader *)next_header_addr) + 1);
}

#endif
