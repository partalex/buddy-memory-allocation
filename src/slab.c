#include "../h/slab.h"
#include "../h/Strukture.h"

typedef struct kmem_cache_s kmem_cache_t;
// struct Slab;

#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)

void kmem_init(void *space, int block_num)
{
    slab_inic(space, block_num); // static slab je inicijalizovan
    return;
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size,
                                void (*ctor)(void *),
                                void (*dtor)(void *)) // Allocate cache
{
    return kes_alloc(name, size, ctor, dtor);
}
void kmem_cache_destroy(kmem_cache_t *cachep) // Deallocate cache
{
    return kes_free(cachep);
}
void *kmem_cache_alloc(kmem_cache_t *cachep) // Allocate one object from cache
{
    return obj_alloc(cachep);
}
void kmem_cache_free(kmem_cache_t *cachep, void *objp) // Deallocate one object from cache
{
    return obj_free(cachep, objp);
}
void *kmalloc(size_t size) // Alloacate one small memory buffer
{
    return buff_alloc(size);
}
void kfree(const void *objp) // Deallocate one small memory buffer
{
    return (objp);
}
int kmem_cache_shrink(kmem_cache_t *cachep) // Shrink cache
{
    return skupi_kes(cachep);
}
void kmem_cache_info(kmem_cache_t *cachep) // Print cache info
{
    return (cachep);
}
int kmem_cache_error(kmem_cache_t *cachep) // Print error message
{
    return kes_error(cachep);
}