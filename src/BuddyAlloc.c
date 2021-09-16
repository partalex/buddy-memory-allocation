#include "../h/slab.h"
#include "../h/Strukture.h"
#include "../h/lockic.h"
#include <math.h>

void buddy_inic()
{
	buddy->mutex = create_mutex();
	uintptr_t adress = buddy->pocetna_adesa;
	unsigned index;
	unsigned nedodeljeni_blokovi = buddy->broj_blokova;
	while (nedodeljeni_blokovi)
	{
		index = (unsigned)floor(log2((double)nedodeljeni_blokovi));
		Buddy_block* bud = adress;
		buddy->niz_slobodnih_blokova[index] = adress;
		bud->sledeci = NULL;
		bud->local = 1 << index;
		adress += (pow(2, index) * BLOCK_SIZE);
		nedodeljeni_blokovi -= (unsigned)pow(2, index);
	}
}

unsigned bajtove_u_min_blokova(unsigned broj_bajtova)
{
	return (unsigned)ceil(1. * broj_bajtova / BLOCK_SIZE);
}

unsigned min_stepen_za_broj_blokova(unsigned broj_blokova)
{
	return (unsigned)ceil(log2((double)broj_blokova));
}

unsigned podeli_blok(unsigned index)
{
	Buddy_block* prvi_buddy = buddy->niz_slobodnih_blokova[index]; // uzmi prvi koji se deli
	buddy->niz_slobodnih_blokova[index] = prvi_buddy->sledeci;     // izbaci onog kog delim

	Buddy_block* next = buddy->niz_slobodnih_blokova[index - 1];
	buddy->niz_slobodnih_blokova[index - 1] = prvi_buddy;

	Buddy_block* drugi_buddy = (Buddy_block*)((uintptr_t)prvi_buddy + (unsigned)(pow(2, index - 1) * BLOCK_SIZE));
	drugi_buddy->sledeci = NULL;
	drugi_buddy->local = prvi_buddy->local | (1 << index - 1);
	prvi_buddy->sledeci = drugi_buddy;

	return index - 1;
}

void premesti_slab(Slab_block* slab_block, unsigned iz_praznog_slaba)
{
	Kes* kes = slab_block->header.moj_kes;
	/*if (!kes->prazan && iz_praznog_slaba)
		iz_praznog_slaba = 0;*/

	if (iz_praznog_slaba)
	{ // potice iz praznog
		Slab_block* prazan = kes->prazan;
		Slab_block* prev_prazan = NULL;
		while (prazan) {
			if (prazan == slab_block)
				break;
			prev_prazan = prazan;
			prazan = prazan->header.sledeci;
		}
		if (!prev_prazan) {
			if (!kes->prazan)
				printf("tu is");
			kes->prazan = kes->prazan->header.sledeci;
		}
		else
			prev_prazan->header.sledeci = prazan->header.sledeci;
	}
	else
	{ // potice iz nepunog
		if (slab_block->header.broj_slobodnih_slotova)
			return;
		Slab_block* nepun = kes->nepun;
		Slab_block* prev_nepun = NULL;
		while (nepun) {
			if (nepun == slab_block)
				break;
			prev_nepun = nepun;
			nepun = nepun->header.sledeci;
		}
		if (!prev_nepun)
			kes->nepun = kes->nepun->header.sledeci;
		else
			prev_nepun->header.sledeci = nepun->header.sledeci;

	}
	if (slab_block->header.broj_slobodnih_slotova)
	{ // staviti ga u nepune
		Slab_block* nepun = kes->nepun;
		Slab_block* prev_nepun = NULL;

		while (nepun) {
			if (slab_block->header.broj_slobodnih_slotova > nepun->header.broj_slobodnih_slotova)
				break;
			prev_nepun = nepun;
			nepun = nepun->header.sledeci;
		}
		if (!nepun)
			kes->nepun = slab_block;
		else if (!prev_nepun)
		{ //slab_block->header.sledeci = 
			kes->nepun->header.sledeci = slab_block;
			return;
		}
		else {
			slab_block->header.sledeci = prev_nepun->header.sledeci;
			prev_nepun->header.sledeci = slab_block;
			return;
		}
	}
	else
	{ // staviti ga u pune
		Slab_block* pun = kes->pun;
		Slab_block* prev_pun = NULL;

		if (kes->pun)
			slab_block->header.sledeci = kes->pun;
		kes->pun = slab_block;
	}
}

void* slot_alloc(Slab_block* slab_block, unsigned* iz_praznog_slaba)
{
	uintptr_t prazan_slot = slab_block->header.prvi_slot;

	for (size_t i = 0; i < slab_block->header.broj_slotova; i++)
	{
		if (*(char*)prazan_slot == 0)
		{
			slab_block->header.broj_slobodnih_slotova--;
			premesti_slab(slab_block, *iz_praznog_slaba);
			return prazan_slot;
		}
		prazan_slot += slab_block->header.velicina_slota;
	}
	return NULL;
}

void slab_info(Slab_block* slab_block) {
	if (!slab_block)
		printf("Does not exist\n");

	Slab_block* next = slab_block;
	while (next) {
		printf("Address of slab : %p\n", next);
		printf("Size of slab : %d\n", next->header.stepen_dvojke);
		printf("Address of first slot : %zd\n", next->header.prvi_slot);
		printf("Size of slot : %d\n", next->header.velicina_slota);
		printf("Number of slots : %d\n", next->header.broj_slotova);
		printf("Number of free slots : %d\n", next->header.broj_slobodnih_slotova);
		printf("Number of slots : %d\n", next->header.stepen_dvojke);
		printf("Address of next slab : %p\n", next->header.sledeci);
		next = next->header.sledeci;
	}
}

Slab_block* slab_alloc_typed(Kes* moj_kes)
{
	unsigned potrebno_blokova = bajtove_u_min_blokova(moj_kes->velicina + sizeof(Slab_block_header));
	unsigned min_stepen = min_stepen_za_broj_blokova(potrebno_blokova);

	unsigned sledeci_stepen = min_stepen;
	while (sledeci_stepen < VELICINA_PAMTLJIVOG) {
		if (buddy->niz_slobodnih_blokova[sledeci_stepen])
			break;
		sledeci_stepen++;
	}
	if (sledeci_stepen == VELICINA_PAMTLJIVOG)
		return NULL; // nema slobodnih
	while (sledeci_stepen < VELICINA_PAMTLJIVOG) // radi deobu
	{
		if (sledeci_stepen == min_stepen)
		{
			Buddy_block* slobodan = buddy->niz_slobodnih_blokova[min_stepen];
			unsigned short local = slobodan->local;
			buddy->niz_slobodnih_blokova[min_stepen] = slobodan->sledeci;
			slobodan->sledeci = NULL;
			Slab_block* ret = (Slab_block*)slobodan;
			ret->header.stepen_dvojke = min_stepen;
			ret->header.sledeci = NULL;
			ret->header.local = local;
			ret->header.velicina_slota = moj_kes->velicina;
			ret->header.prvi_slot = (uintptr_t)ret + sizeof(Slab_block_header);
			ret->header.broj_slotova = (unsigned)(pow(2, ret->header.stepen_dvojke) * BLOCK_SIZE) - sizeof(Slab_block_header);
			ret->header.broj_slotova /= moj_kes->velicina;
			ret->header.broj_slobodnih_slotova = ret->header.broj_slotova;
			memset(ret->header.prvi_slot, 0, ret->header.broj_slobodnih_slotova * ret->header.velicina_slota);
			ret->header.moj_kes = moj_kes;
			return ret;
		}
		sledeci_stepen = podeli_blok(sledeci_stepen);
	}
	return NULL;
}

Slab_block* slab_alloc_buffered(Kes* moj_kes)
{
	unsigned potrebno_blokova = bajtove_u_min_blokova(moj_kes->velicina);
	unsigned min_stepen = min_stepen_za_broj_blokova(potrebno_blokova);

	unsigned sledeci_stepen = min_stepen;
	while (sledeci_stepen < VELICINA_PAMTLJIVOG) {
		if (buddy->niz_slobodnih_blokova[sledeci_stepen])
			break;
		sledeci_stepen++;
	}
	if (sledeci_stepen == VELICINA_PAMTLJIVOG)
		return NULL; // nema slobodnih

	while (sledeci_stepen < VELICINA_PAMTLJIVOG) // radi deobu
	{
		if (sledeci_stepen == min_stepen)
		{
			Buddy_block* slobodan = buddy->niz_slobodnih_blokova[min_stepen];
			buddy->niz_slobodnih_blokova[min_stepen] = slobodan->sledeci;
			slobodan->sledeci = NULL;
			Slab_block* ret = (Slab_block*)slobodan;
			memset(ret, 0, sizeof(Slab_block));
			ret->header.stepen_dvojke = min_stepen;
			ret->header.velicina_slota = moj_kes->velicina - sizeof(Slab_block_header);
			ret->header.prvi_slot = (uintptr_t)ret + sizeof(Slab_block_header);;
			ret->header.broj_slotova = 1;
			ret->header.broj_slobodnih_slotova = 1;
			memset(ret->header.prvi_slot, 0, ret->header.broj_slobodnih_slotova * ret->header.velicina_slota);
			ret->header.moj_kes = moj_kes;
			return ret;
		}
		sledeci_stepen = podeli_blok(sledeci_stepen);
	}
	return NULL;

}

Buddy_block* spoji_ako_je_brat_slobodan(Buddy_block* buddy_brat, size_t* stepen_dvojke)
{
	//memset((uintptr_t)buddy_brat + sizeof(Buddy_block), 0, pow(2, *stepen_dvojke) - sizeof(Buddy_block));
	Buddy_block* next = buddy->niz_slobodnih_blokova[*stepen_dvojke];
	if (!next) {
		buddy->niz_slobodnih_blokova[*stepen_dvojke] = buddy_brat;
		return NULL;
	}
	Buddy_block* prethodni = NULL;
	while (next) {
		unsigned short mask = 1 << *stepen_dvojke;
		mask &= buddy_brat->local;
		mask >>= *stepen_dvojke;
		if (!mask)
			mask = buddy_brat->local + pow(2, *stepen_dvojke);
		else
			mask = buddy_brat->local - pow(2, *stepen_dvojke);

		if (next->local == mask)
			break;
		prethodni = next;
		next = next->sledeci;
	}
	if (!next)
		return NULL;
	if (prethodni) {
			prethodni->sledeci = next->sledeci;
	}
	else
		buddy->niz_slobodnih_blokova[*stepen_dvojke] = next->sledeci;
	(*stepen_dvojke)++;
	if ((uintptr_t)buddy_brat < (uintptr_t)next)
		return buddy_brat;
	else
		return next;
}

unsigned oslobodi(Buddy_block* buddy_block, size_t stepen_dvojke)
{
	Buddy_block* next = spoji_ako_je_brat_slobodan(buddy_block, &stepen_dvojke);
	while (next)
	{
		next = spoji_ako_je_brat_slobodan(next, &stepen_dvojke);
	}
	return (unsigned)stepen_dvojke;
}
