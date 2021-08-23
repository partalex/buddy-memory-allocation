#include "../h/slab.h"
#include "../h/Strukture.h"
#include <math.h>

void buddy_inic()
{
    unsigned nedodeljeni_blokovi = buddy->broj_blokova;
    while (nedodeljeni_blokovi)
    {
        unsigned index = floor(log2((double)nedodeljeni_blokovi));
        unsigned adress = (unsigned)(buddy->pocetna_adesa) + pow(2, index) * BLOCK_SIZE;
        buddy->niz_slobodnih_blokova[index] = (Buddy_block *)adress;
        buddy->niz_slobodnih_blokova[index]->sledeci = NULL;
        nedodeljeni_blokovi -= pow(2, index);
    }
}

unsigned bajtove_u_min_blokova(unsigned broj_bajtova)
{
    return ceil(1.* broj_bajtova / BLOCK_SIZE);
}

unsigned min_stepen_za_broj_blokova(unsigned broj_blokova)
{
    return ceil(log2((double)broj_blokova));
}

unsigned podeli_blok(unsigned index)
{
    Buddy_block *prvi_buddy = buddy->niz_slobodnih_blokova[index]; // uzmi prvi koji se deli
    buddy->niz_slobodnih_blokova[index] = prvi_buddy->sledeci;     // izbaci onaj kog delim

    Buddy_block *next = buddy->niz_slobodnih_blokova[index - 1];
    while (next->sledeci)
        next = next->sledeci;
    next->sledeci = prvi_buddy;
    Buddy_block *drugi_buddy = (Buddy_block *)((unsigned)prvi_buddy + (unsigned)(pow(2, index - 1) * BLOCK_SIZE));

    prvi_buddy->sledeci = drugi_buddy;

    buddy->niz_slobodnih_blokova[index - 1];

    return index - 1;
}

struct s_Slab_block *zauzmi(size_t potrebno_bajtova, unsigned velicina_slota)
{
    unsigned potrebno_blokova = bajtove_u_min_blokova(potrebno_bajtova);
    unsigned min_stepen = min_stepen_za_broj_blokova(potrebno_blokova);
    Buddy_block *slobodan = NULL;
    Slab_block *ret;

    unsigned sledeci_stepen = min_stepen;
    while (sledeci_stepen < VELICINA_PAMTLJIVOG)
    {
        if (sledeci_stepen == min_stepen)
        {
            slobodan = buddy->niz_slobodnih_blokova[min_stepen];
            buddy->niz_slobodnih_blokova[min_stepen] = slobodan->sledeci;
            slobodan->sledeci = NULL;
            ret = (Slab_block *)slobodan;
            memset(ret, 0, sizeof(Slab_block));
            ret->header.stepen_dvojke = min_stepen;
            ret->header.velicina_slota = velicina_slota;
            ret->header.prvi_slot = (void*)((unsigned)ret + sizeof(Slab_block_header));
            ret->header.broj_slotova = (unsigned)(pow(2, ret->header.stepen_dvojke)*BLOCK_SIZE) - sizeof(Slab_block_header);
            ret->header.broj_slotova /= velicina_slota;
            return ret;
        }
        if (buddy->niz_slobodnih_blokova[sledeci_stepen])
        {
            sledeci_stepen = podeli_blok(sledeci_stepen, buddy);
            continue;
        }
        else
        {
            sledeci_stepen++;
        }
    }
    return NULL;
}

Buddy_block *spoji_ako_je_brat_slobodan(Buddy_block *buddy_brat, size_t *stepen_dvojke)
{
    Buddy_block *next = buddy->niz_slobodnih_blokova[*stepen_dvojke];
    Buddy_block *prethodni = NULL;
    Buddy_block *brat = NULL;
    unsigned stepen = pow(2, *stepen_dvojke);
    unsigned moguci_nizi_brat = (unsigned)buddy_brat - stepen * BLOCK_SIZE;
    unsigned moguci_visi_brat = (unsigned)buddy_brat + stepen * BLOCK_SIZE;
    short visi = 0; // visi = 1, nizi = 0
    while (next)
    {
        if ((unsigned)next == moguci_nizi_brat)
        {
            brat = next;
            break;
        }
        else if ((unsigned)next == moguci_visi_brat)
        {
            visi = 1;
            brat = next;
            break;
        }
        prethodni = next;
        next = next->sledeci;
    }
    if (brat) // nasao brata
    {
        if (prethodni)
        {
            prethodni->sledeci = next->sledeci;
        }
        else
        {
            buddy->niz_slobodnih_blokova[*stepen_dvojke] = next->sledeci;
        }
        next = buddy->niz_slobodnih_blokova[++(*stepen_dvojke)];
        while (next->sledeci)
            next = next->sledeci;
        if (visi)
        {
            next->sledeci = buddy_brat;
            next->sledeci->sledeci = NULL;
            return buddy_brat;
        }
        else
        {
            next->sledeci = (Buddy_block *)moguci_nizi_brat;
            next->sledeci->sledeci = NULL;
            return (Buddy_block *)moguci_nizi_brat;
        }
    }
    return NULL; // nije uspeo da uradi merge brace, tj. nije nasao brata
}

unsigned oslobodi(Buddy_block *buddy_block, size_t stepen_dvojke)
{
    Buddy_block *next = spoji_ako_je_brat_slobodan(buddy_block, &stepen_dvojke);
    while (next)
    {
        next = spoji_ako_je_brat_slobodan(next, &stepen_dvojke);
    }
    return stepen_dvojke;
}
