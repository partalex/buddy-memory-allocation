#include "../h/Strukture.h"
#include "../h/slab.h"

void buddy_inicijalizacija(Buddy *buddy)
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
    return ceil(broj_bajtova / BLOCK_SIZE);
}

unsigned min_stepen_za_broj_blokova(unsigned broj_blokova)
{
    return ceil(log2((double)broj_blokova));
}

unsigned podeli_blok(unsigned index, Buddy *buddy)
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

Buddy_block *zauzmi(size_t potrebno_bajtova, Buddy *buddy)
{
    unsigned potrebno_blokova = bajtove_u_min_blokova(potrebno_bajtova);
    unsigned min_stepen = min_stepen_za_broj_blokova(potrebno_blokova);
    Buddy_block *slobodan = NULL;

    unsigned sledeci_stepen = min_stepen;
    while (sledeci_stepen < VELICINA_PAMTLJIVOG)
    {
        if (sledeci_stepen == min_stepen)
        {
            slobodan = buddy->niz_slobodnih_blokova[min_stepen];
            buddy->niz_slobodnih_blokova[min_stepen] = slobodan->sledeci;
            slobodan->sledeci = NULL;
            return slobodan;
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
    return slobodan;
}

unsigned oslobodi(Buddy_block *buddy_block, size_t broj_bajtova, Buddy *buddy)
{
    unsigned pocetna_adresa = 0;
    unsigned adresa_za_oslobajdja = (unsigned)buddy_block - (unsigned)(buddy->pocetna_adesa);

    return 1;
}
