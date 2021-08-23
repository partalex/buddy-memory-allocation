#include "../h/slab.h"
#include "../h/Strukture.h"

typedef struct kmem_cache_s kmem_cache_t;
// struct Slab;

#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)

void kmem_init(void *space, int block_num)
{
    Slab *slab = slab_inicijalizacija(space, block_num);
    return;
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size,
                                void (*ctor)(void *),
                                void (*dtor)(void *)) // Allocate cache
{
    // kmem_cache_t *kes = napravi_kes(name, size, ctor, dtor);
    // return kes;
    return NULL;
}
int kmem_cache_shrink(kmem_cache_t *cachep) // Shrink cache
{
    return 1;
}
void *kmem_cache_alloc(kmem_cache_t *cachep) // Allocate one object from cache
{
    return NULL;
}
void kmem_cache_free(kmem_cache_t *cachep, void *objp) // Deallocate one object from cache
{
}
void *kmalloc(size_t size) // Alloacate one small memory buffer
{
    return NULL;
}
void kfree(const void *objp) // Deallocate one small memory buffer
{
}
void kmem_cache_destroy(kmem_cache_t *cachep) // Deallocate cache
{
}
void kmem_cache_info(kmem_cache_t *cachep) // Print cache info
{
}
int kmem_cache_error(kmem_cache_t *cachep) // Print error message
{
    return 1;
}