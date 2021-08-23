#include "../h/slab.h"
#include "../h/Strukture.h"

typedef struct kmem_cache_s kmem_cache_t;
// struct Slab;

#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)

void kmem_init(void *space, int block_num)
{
    unsigned sizeof_Slab = sizeof(Slab);
    unsigned sizeof_Cashe = sizeof(kmem_cache_t);

    Slab *slab = (Slab *)space;
    slab->ulancani_kesevi = (kmem_cache_t **)((unsigned)space + sizeof(Slab));
    *slab->ulancani_kesevi = NULL; // prazna lista keseva

    slab->buddy.broj_blokova = (unsigned)(block_num - (unsigned)ceil((sizeof(Slab) * 1. + _0_PROSTOR_ZA_KESEVE) / BLOCK_SIZE));
    slab->buddy.broj_ulaza = (unsigned)(floor(log2((double)slab->buddy.broj_blokova)) + 1);
    slab->buddy.pocetna_adesa = (void *)((unsigned)space + (unsigned)(ceil(1. * _0_PROSTOR_ZA_KESEVE / BLOCK_SIZE) * BLOCK_SIZE));
    buddy_inicijalizacija(&slab->buddy);

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