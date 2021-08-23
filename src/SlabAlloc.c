#include <string.h>
#include "../h/slab.h"
#include "../h/Strukture.h"
#define BLOCK_SIZE (4096)

void inic_tipske_keseve()
{
    char *naziv_start = "Mem Buffer ";
    char *broj = 3;
    for (size_t i = 0; i < 13; i++)
    {
        slab->baferisani_kesevi[i].ctor = NULL;
        slab->baferisani_kesevi[i].dtor = NULL;

        slab->baferisani_kesevi[i].naziv = strcat_s(naziv_start, 10, _itoa_s(i + 5, broj, 1, 10));
        slab->baferisani_kesevi[i].velicina = i + 5;

        slab->baferisani_kesevi[i].prazan = NULL;
        slab->baferisani_kesevi[i].nepun = NULL;
        slab->baferisani_kesevi[i].pun = NULL;

        slab->baferisani_kesevi[i].sledeci = NULL;
    }
}

void inic_baferske_keseve(kmem_cache_t *prvi_kes, unsigned broj_keseva)
{
    kmem_cache_t *temp = prvi_kes;
    for (size_t i = 0; i < broj_keseva; i++)
    {
        memset(temp, 0, sizeof(kmem_cache_t));
        temp++;
    }
}

void slab_inic(void *pocetna_adresa, unsigned ukupan_broj_blokova)
{
    slab = (Slab *)pocetna_adresa;
    inic_tipske_keseve();

    slab->ulancani_kesevi = (kmem_cache_t *)((unsigned)pocetna_adresa + sizeof(Slab)); // prazna lista keseva
    inic_baferske_keseve(slab->ulancani_kesevi, BROJ_TIPSKIH_KESEVA);
    memset(slab->niz_napravljenih_keseva, 0, BROJ_TIPSKIH_KESEVA * sizeof(unsigned char));

    slab->buddy.broj_blokova = (unsigned)(ukupan_broj_blokova - (unsigned)ceil((sizeof(Slab) * 1.) / BLOCK_SIZE)); // ova linija moze da bude promeljiva
    slab->buddy.pocetna_adesa = (Buddy *)((unsigned)pocetna_adresa + (unsigned)ceil(((sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(kmem_cache_t)) * 1. / BLOCK_SIZE)));

    buddy = &slab->buddy;
    buddy_inic();
}

kmem_cache_t *daj_prazno_mesto_za_kes()
{
    for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
        if (!slab->niz_napravljenih_keseva[i])
        {
            slab->niz_napravljenih_keseva[i] = 1;
            return slab->ulancani_kesevi + i;
        }
    return NULL;
}

kmem_cache_t *kes_alloc(const char *naziv, size_t velicina, // size je broj blokova ili velcina objekta tog tog tipa
                        void (*ctor)(void *), void (*dtor)(void *))
{
    kmem_cache_t *kes = daj_prazno_mesto_za_kes();
    if (!kes)
        return NULL; // nema mesta vise

    kes->naziv = naziv;
    kes->velicina = velicina;
    kes->ctor = ctor;
    kes->dtor = dtor;

    return NULL;
}

void oslobodi_slabove_kesa(kmem_cache_t *kes)
{
    Slab_block *next = kes->nepun;
    Slab_block *za_oslobadjanje = NULL;
    while (next)
    {
        za_oslobadjanje = next;
        next = next->header.sledeci;
        oslobodi((Buddy_block *)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
    }
    next = kes->pun;
    while (next)
    {
        za_oslobadjanje = next;
        next = next->header.sledeci;
        oslobodi((Buddy_block *)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
    }
    next = kes->prazan;
    while (next)
    {
        za_oslobadjanje = next;
        next = next->header.sledeci;
        oslobodi((Buddy_block *)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
    }
}

void kes_free(kmem_cache_t *kes)
{
    for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
        if (kes == slab->ulancani_kesevi + i)
            slab->niz_napravljenih_keseva[i] = 0;

    // doraditi : osloboditi slabove
    oslobodi_slabove_kesa(kes);
}

Slab_block *obezbedi_slab_za_objekat(kmem_cache_t *kes)
{
    if (!kes->nepun)
    {
        if (!kes->prazan)
        {
            kes->nepun = zauzmi(pow(2, kes->velicina), kes->velicina); // vraca prazan blcok
            if (!kes->nepun)
                return NULL;
            return kes->nepun;
        }
    }
    return kes->nepun;
}

void *obj_alloc(kmem_cache_t *kes) // vraca obj tipa koji kes cuva
{
    Slab_block *slab_block = obezbedi_slab_za_objekat(kes);
    if (!slab_block)
        return NULL;



    return NULL;
}

void obj_free(kmem_cache_t *kes, void *obj)
{
}

void *buff_alloc(size_t size)
{
    return NULL;
}

void buff_free(const void *buff)
{
}

kmem_cache_t *skupi_kes(kmem_cache_t *kes)
{
    return NULL;
}

void kes_info(kmem_cache_t *kes)
{
}

int kes_error(kmem_cache_t *kes)
{
    return 1;
}
