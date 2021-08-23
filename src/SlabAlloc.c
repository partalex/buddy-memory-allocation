#include "../h/Strukture.h"
#define BLOCK_SIZE (4096)
typedef struct kmem_cache_s kmem_cache_t;

Slab *slab_inicijalizacija(void *pocetna_adresa, unsigned ukupan_broj_blokova)
{
    Slab *slab = (Slab *)pocetna_adresa;
    slab->ulancani_kesevi = (kmem_cache_t *)((unsigned)pocetna_adresa + sizeof(Slab)); // prazna lista keseva
    slab->buddy.broj_blokova = (unsigned)(ukupan_broj_blokova - (unsigned)ceil((sizeof(Slab) * 1.) / BLOCK_SIZE)); // ova linija moze da bude promeljiva
    slab->buddy.pocetna_adesa = (Buddy *)((unsigned)pocetna_adresa + (unsigned)ceil(((sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(kmem_cache_t))*1./BLOCK_SIZE)));
    buddy_inicijalizacija(&slab->buddy);

    return slab;
}
