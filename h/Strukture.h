#pragma once

#include <math.h>

#define VELICINA_PAMTLJIVOG (10) // ovo bi trebalo da se izracuna
// #define velicna_pamtljivog (floor(log2((double)995)) + 1) // ovo bi trebalo da se izracuna

/////////////////////////////************************/////////////////////////////
/////////////////////////////*******  Buddy  ********/////////////////////////////
/////////////////////////////************************/////////////////////////////

typedef struct s_Buddy_block
{
    struct s_Buddy_block *sledeci;
} Buddy_block;

typedef struct
{
    void *pocetna_adesa;
    Buddy_block *niz_slobodnih_blokova[VELICINA_PAMTLJIVOG];
    // void **niz_slobodnih_blokova; // ovo bi trebalo da bude struktaura
    unsigned broj_blokova;
    unsigned broj_ulaza;
} Buddy;

void buddy_inicijalizacija(Buddy *buddy);
unsigned bajtove_u_min_blokova(unsigned broj_bajtova);
unsigned min_stepen_za_broj_blokova(unsigned broj_blokova);
unsigned podeli_blok(unsigned index, Buddy *buddy);
// unsigned moguca_adresa_brata(Buddy_block *buddy, size_t stepen_dvojke, short buddy_ispod);
Buddy_block *spoji_ako_je_brat_slobodan(Buddy_block *buddy_brat, size_t *stepen_dvojke, Buddy *buddy); // vrati adresu spojenog ili NULL ako nije nasao nista
Buddy_block *zauzmi(size_t potrebno_bajtova, Buddy *buddy);
unsigned oslobodi(Buddy_block *buddy_block, size_t pottrebno_bajtova, Buddy *buddy); // vraca stepen dvojke oslobodjenog(mergovanog) bloka

/////////////////////////////************************/////////////////////////////
/////////////////////////////****  Slab i Cashe  ****/////////////////////////////
/////////////////////////////************************/////////////////////////////

typedef struct s_Slab_block
{
    struct s_Slab_block *sledeci;
    unsigned prvi_slot;
    unsigned broj_slotova;

    // struct kmem_cache_s* pripadam_kesu;

} Slab_block;

struct kmem_cache_s
{
    void (*ctor)(void *);
    void (*dtor)(void *);

    const char *naziv;
    unsigned int velicina;

    Slab_block *prazan;
    Slab_block *nepun;
    Slab_block *pun;

    struct kmem_cache_s *sledeci; // pokazivac
};

#define VELICINA_KESA (sizeof(struct kmem_cache_s))
#define BROJ_RAZLICITIH_KESEVA (13)
#define _0_PROSTOR_ZA_KESEVE (VELICINA_KESA * BROJ_RAZLICITIH_KESEVA)

typedef struct s_Slab
{
    Buddy buddy;
    struct kmem_cache_s baferisani_kesevi[BROJ_RAZLICITIH_KESEVA];
    struct kmem_cache_s **ulancani_kesevi; // nalazi se na mestu ispod slaba da moze se redja
    // unsigned rezervisani_broj_keseva;

} Slab;

Slab *slab_inicijalizacija(void *);