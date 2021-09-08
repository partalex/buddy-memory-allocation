#pragma once

#include <math.h>
#include <stdio.h>

// typedef struct kmem_cache_s kmem_cache_t;
typedef struct kmem_cache_s kmem_cache_t;
typedef struct kmem_cache_s Kes;
typedef struct s_Slab_block Slab_block;
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
    unsigned broj_blokova;
} Buddy;

void buddy_inic();
unsigned bajtove_u_min_blokova(unsigned broj_bajtova);
unsigned min_stepen_za_broj_blokova(unsigned broj_blokova);
unsigned podeli_blok(unsigned index);
Buddy_block *spoji_ako_je_brat_slobodan(Buddy_block *buddy_brat, size_t *stepen_dvojke); // vrati adresu spojenog ili NULL ako nije nasao nista
void premesti_slab(Slab_block *slab_block, unsigned char *iz_praznog_slaba, Kes *kes);
void *slot_alloc(Slab_block *slab_block, unsigned char *iz_praznog_slaba);
void slot_free(Kes *kes);
Slab_block*zauzmi(Kes *moj_kes);
unsigned oslobodi(Buddy_block *buddy_block, size_t pottrebno_bajtova); // vraca stepen dvojke oslobodjenog(mergovanog) bloka

/////////////////////////////************************/////////////////////////////
/////////////////////////////****  Slab i Cashe  ****/////////////////////////////
/////////////////////////////************************/////////////////////////////

typedef struct s_Slab_block_header
{
    struct s_Slab_block *sledeci;
    uintptr_t prvi_slot;
    unsigned broj_slotova;
    unsigned velicina_slota;
    unsigned broj_slobodnih_slotova;
    unsigned short stepen_dvojke;
    Kes *moj_kes;
} Slab_block_header;

struct s_Slab_block
{
    Slab_block_header header;
};

struct kmem_cache_s
{
    void (*ctor)(void *);
    void (*dtor)(void *);

    const char *naziv;
    unsigned int velicina;

    Slab_block *prazan;
    Slab_block *nepun;
    Slab_block *pun;

    Kes *sledeci; // pokazivac
};

#define BROJ_KESEVA_ZA_BAFERE (13)
#define BLOCK_SIZE (4096)
#define BROJ_TIPSKIH_KESEVA (109)

typedef struct s_Slab
{
    Buddy buddy;
    Kes baferisani_kesevi[BROJ_KESEVA_ZA_BAFERE];
    Kes *ulancani_kesevi; // nalazi se na mestu ispod slaba da moze se redja
    unsigned char niz_napravljenih_keseva[BROJ_TIPSKIH_KESEVA];

} Slab;

Slab *slab;
Buddy *buddy;

// enkapsulirane f-je

void slab_inic(uintptr_t pocetna_adresa, unsigned ukupan_broj_blokova);
Kes *kes_alloc(const char *naziv, size_t velicina, // size je broj blokova ili velcina objekta tog tog tipa
               void (*ctor)(void *), void (*dtor)(void *));
void kes_free(Kes *kes);
void *obj_alloc(Kes *kes); // vraca obj tipa koji kes `cuva
void obj_free(Kes *kes, void *obj);
void *buff_alloc(size_t size);
void buff_free(const void *buff); // nemam kontrolu pogresnog unosa
Kes *skupi_kes(Kes *kes);
void kes_info(Kes *kes);
int kes_error(Kes *kes);

// moje pomocne f-je
int index_tipskog_kesa(Kes *kes);
void inic_baferske_keseve();
void inic_tipske_keseve();
Kes *daj_prazno_mesto_za_kes();
void oslobodi_slabove_kesa(Kes *kes);
Slab_block *obezbedi_slab_za_objekat(Kes *kes, unsigned char *iz_praznog_slaba);
void preuredi_nepune_slabove(Kes *kes);
Slab_block *pronadji_slab_objekta_kog_brises(Kes *kes, void *obj);
Slab_block *obrisi_objekt_u_slabu(Slab_block *slab_block, void *obj);
