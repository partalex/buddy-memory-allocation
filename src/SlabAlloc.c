#include <string.h>
#include "../h/slab.h"
#include "../h/Strukture.h"
#define BLOCK_SIZE (4096)

int index_tipskog_kesa(Kes *kes)
{
    for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
        if (kes == slab->ulancani_kesevi + i)
            return i;
    return -1;
}

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

void inic_baferske_keseve(Kes *prvi_kes, unsigned broj_keseva)
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

Kes *daj_prazno_mesto_za_kes()
{
    for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
        if (!slab->niz_napravljenih_keseva[i])
        {
            slab->niz_napravljenih_keseva[i] = 1;
            return slab->ulancani_kesevi + i;
        }
    return NULL;
}

Kes *kes_alloc(const char *naziv, size_t velicina, // size je broj blokova ili velcina objekta tog tog tipa
               void (*ctor)(void *), void (*dtor)(void *))
{
    kmem_cache_t *kes = daj_prazno_mesto_za_kes();
    if (!kes)
        return NULL; // nema mesta vise

    kes->naziv = naziv;
    kes->velicina = velicina;
    kes->ctor = ctor;
    kes->dtor = dtor;

    kes->prazan = zauzmi(pow(2, kes->velicina), kes->velicina, kes);
    if (!kes->prazan)
        return NULL;

    return kes;
}

void oslobodi_slabove_kesa(Kes *kes)
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

void kes_free(Kes *kes)
{
    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return;
    slab->niz_napravljenih_keseva[index] = 0;
    oslobodi_slabove_kesa(kes);
}

Slab_block *obezbedi_slab_za_objekat(Kes *kes, unsigned char *iz_praznog_slaba)
{
    if (!kes->nepun)
        if (kes->prazan)
        {
            *iz_praznog_slaba = 1;
            return kes->prazan;
        }
        else
            return kes->prazan = zauzmi(pow(2, kes->velicina), kes->velicina, kes);
    return kes->nepun;
}

void *obj_alloc(Kes *kes) // vraca obj tipa koji kes cuva
{
    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return NULL;
    unsigned char iz_praznog_slaba = 1;
    Slab_block *slab_block = obezbedi_slab_za_objekat(kes, &iz_praznog_slaba);
    if (!slab_block)
        return NULL;
    void *slot = slot_alloc(slab_block, &iz_praznog_slaba);
    if (!slot)
        return NULL;
    kes->ctor(slot);
    return slot;
}

Slab_block *obrisi_objekt_u_slabu(Slab_block *slab_block, void *obj)
{
    unsigned prazan_slot = slab_block->header.prvi_slot;
    for (size_t i = 0; i < slab_block->header.broj_slotova; i++)
    {
        prazan_slot += i * slab_block->header.velicina_slota;
        if (!memcmp((void *)prazan_slot, obj, slab_block->header.velicina_slota))
        {
            slab_block->header.moj_kes->dtor(obj);
            memset(obj, 0, slab_block->header.velicina_slota);
            slab_block->header.broj_slobodnih_slotova++;
            return slab_block;
        }
    }
    return NULL;
}

void preuredi_nepune_slabove(Kes *kes)

{
    Slab_block* prvi = kes->nepun;
    Slab_block* drugi = kes->nepun->header.sledeci;
    void* slot;

    if (!drugi) {
        return; // ima samo jedan nepun, nema smisla premestati
        if (prvi->header.broj_slobodnih_slotova + drugi->header.broj_slobodnih_slotova > prvi->header.broj_slotova)
            return;
        for (size_t i = 0; i < drugi->header.broj_slobodnih_slotova; i++)
        {
            if (*(char*)(drugi->header.prvi_slot + i * drugi->header.velicina_slota) != 0) {
                slot = slot_alloc(prvi, 3, kes);
                if (!slot)
                    printf("problem");
            }
        }
    }
    else {
        prvi->header.sledeci = NULL;
        oslobodi((Buddy_block*)drugi, drugi->header.stepen_dvojke);
    }
}

Slab_block *pronadji_slab_objekta_kog_brises(Kes *kes, void *obj)
{
    Slab_block *ret;
    Slab_block *next = NULL;
    Slab_block *prev;
    if (!kes->nepun)
        if (!kes->pun)
            return NULL;
    //*iz_punog_slaba = 1;
    next = kes->pun;
    prev = NULL;
    while (next->header.sledeci)
    {
        if (ret = obrisi_objekt_u_slabu(next, obj))
        {
            if (prev)
            {
                prev->header.sledeci = next->header.sledeci;
                prev = kes->nepun;
                if (!prev)
                    kes->nepun = next;
                else
                {
                    while (prev->header.sledeci)
                        prev = prev->header.sledeci;
                    prev->header.sledeci = next;
                }
            }
            else
            {
                kes->pun = NULL;
                prev = kes->nepun;
                if (!prev)
                    kes->nepun = next;
                else
                {
                    while (prev->header.sledeci)
                        prev = prev->header.sledeci;
                    prev->header.sledeci = next;
                }
            }
            preuredi_nepune_slabove(kes);
            return ret;
        }
        prev = next;
        next = next->header.sledeci;
    }
    //*iz_punog_slaba = 0;
    next = kes->nepun;
    while (next->header.sledeci)
    {
        if (ret = obrisi_objekt_u_slabu(next, obj))
        {
            preuredi_nepune_slabove(kes);
            return ret;
        }
        next = next->header.sledeci;
    }
    return NULL;
}

void obj_free(Kes *kes, void *obj)
{
    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return; // ne postoji kes
    //unsigned char iz_punog_slaba = 0;
    Slab_block *za_brisanje = pronadji_slab_objekta_kog_brises(kes, obj);
    //if (!za_brisanje)
    //    return NULL; // ne postotji taj objekat unutar ovog kesa
    return;
}

void *buff_alloc(size_t size)
{
    if (size < 5 || size > 17)
        return NULL;
    return obj_alloc(&slab->baferisani_kesevi[size - 5]);
}

void buff_free(const void *buff)
{
    unsigned char iz_punog_slaba = 0;
    Kes *kes;
    for (size_t i = 0; i < BROJ_KESEVA_ZA_BAFERE; i++)
        if (kes = pronadji_slab_objekta_kog_brises(&slab->baferisani_kesevi[i], buff, &iz_punog_slaba))
            return kes;
    // ako ga ne nadje
    return NULL;
}

Kes *skupi_kes(Kes *kes)
{
    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return NULL;
    return kes;
}

void kes_info(Kes *kes)
{
    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return;
}

int kes_error(Kes *kes)
{

    int index = index_tipskog_kesa(kes);
    if (index == -1)
        return 1;
    return 1;
}
