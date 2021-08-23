#include "../h/Strukture.h"
typedef struct kmem_cache_s kmem_cache_t;

Slab *slab_inicijalizacija(void *pocetna_adresa, unsigned ukupa_broj_blokova)
{
    unsigned sizeof_Slab = sizeof(Slab);
    unsigned sizeof_Cashe = sizeof(kmem_cache_t);

    Slab *slab = (Slab *)pocetna_adresa;
    slab->ulancani_kesevi = (kmem_cache_t **)((unsigned)pocetna_adresa + sizeof(Slab));
    *slab->ulancani_kesevi = NULL; // prazna lista keseva

    slab->buddy.broj_blokova = (unsigned)(ukupa_broj_blokova - (unsigned)ceil((sizeof(Slab) * 1. + _0_PROSTOR_ZA_KESEVE) / BLOCK_SIZE));
    slab->buddy.broj_ulaza = (unsigned)(floor(log2((double)slab->buddy.broj_blokova)) + 1);
    slab->buddy.pocetna_adesa = (void *)((unsigned)pocetna_adresa + (unsigned)(ceil(1. * _0_PROSTOR_ZA_KESEVE / BLOCK_SIZE) * BLOCK_SIZE));
    buddy_inicijalizacija(&slab->buddy);

    return slab;
}
