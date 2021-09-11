#include <string.h>
#include "../h/slab.h"
#include "../h/Strukture.h"
#define BLOCK_SIZE (4096)

int index_tipskog_kesa(Kes* kes)
{
	for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
		if (kes == slab->ulancani_kesevi + i)
			return i;
	return -1;
}

void inic_baferske_keseve()
{
	char* naziv_start = "Mem Buffer ";
	for (size_t i = 0; i < 13; i++)
	{
		memset(slab->baferisani_kesevi + i, 0, sizeof(Kes));
		slab->baferisani_kesevi[i].naziv = naziv_start;
		slab->baferisani_kesevi[i].velicina = pow(2, i + 5);
	}
}

void inic_tipske_keseve() {
	memset(slab->ulancani_kesevi, 0, sizeof(Kes) * BROJ_TIPSKIH_KESEVA);
}

void slab_inic(uintptr_t pocetna_adresa, unsigned ukupan_broj_blokova)
{
	slab = (Slab*)pocetna_adresa;
	inic_baferske_keseve();

	slab->ulancani_kesevi = (Kes*)(pocetna_adresa + sizeof(Slab)); // prazna lista keseva
	inic_tipske_keseve();
	memset(slab->niz_napravljenih_keseva, 0, BROJ_TIPSKIH_KESEVA * sizeof(unsigned char));

	slab->buddy.broj_blokova = (unsigned)(ukupan_broj_blokova - (unsigned)ceil(((sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(kmem_cache_t)) * 1.) / BLOCK_SIZE)); // ova linija moze da bude promeljiva

	unsigned koka = sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(kmem_cache_t);

	slab->buddy.pocetna_adesa = (Buddy*)(pocetna_adresa + (uintptr_t)((ukupan_broj_blokova - slab->buddy.broj_blokova) * BLOCK_SIZE));

	buddy = &slab->buddy;
	memset(buddy->niz_slobodnih_blokova, 0, VELICINA_PAMTLJIVOG * sizeof(Buddy_block*));
	buddy_inic();
}

Kes* daj_prazno_mesto_za_kes()
{
	for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
		if (!slab->niz_napravljenih_keseva[i])
		{
			slab->niz_napravljenih_keseva[i] = 1;
			return slab->ulancani_kesevi + i;
		}
	return NULL;
}

Kes* kes_alloc(const char* naziv, size_t velicina, // size je broj blokova ili velcina objekta tog tog tipa
	void (*ctor)(void*), void (*dtor)(void*))
{
	kmem_cache_t* kes = daj_prazno_mesto_za_kes();
	if (!kes)
		return NULL; // nema mesta vise

	kes->naziv = naziv;
	kes->velicina = velicina;
	kes->ctor = ctor;
	kes->dtor = dtor;

	kes->prazan = slab_alloc_typed(kes);
	if (!kes->prazan)
		return NULL;

	return kes;
}

void oslobodi_slabove_kesa(Kes* kes)
{
	Slab_block* next = kes->nepun;
	Slab_block* za_oslobadjanje = NULL;
	while (next)
	{
		za_oslobadjanje = next;
		next = next->header.sledeci;
		oslobodi((Buddy_block*)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
	}
	next = kes->pun;
	while (next)
	{
		za_oslobadjanje = next;
		next = next->header.sledeci;
		oslobodi((Buddy_block*)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
	}
	next = kes->prazan;
	while (next)
	{
		za_oslobadjanje = next;
		next = next->header.sledeci;
		oslobodi((Buddy_block*)za_oslobadjanje, za_oslobadjanje->header.stepen_dvojke);
	}
}

void kes_free(Kes* kes)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return;
	slab->niz_napravljenih_keseva[index] = 0;
	oslobodi_slabove_kesa(kes);
}

Slab_block* obezbedi_slab_za_typed_obj(Kes* kes, unsigned char* iz_praznog_slaba)
{
	if (!kes->nepun) {
		if (kes->prazan)
		{
			*iz_praznog_slaba = 1;
			return kes->prazan;
		}
		else
			return kes->prazan = slab_alloc_typed(kes);
	}
	*iz_praznog_slaba = 0;
	return kes->nepun;
}

Slab_block* obezbedi_slab_za_buff_obj(Kes* kes) {
	if (kes->prazan)
		return kes->prazan;
	return kes->prazan = slab_alloc_buffered(kes);
}

void* obj_type_alloc(Kes* kes) // vraca obj tipa koji kes cuva
{
	if (index_tipskog_kesa(kes) == -1)
		return NULL;
	unsigned char iz_praznog_slaba = 1;
	Slab_block* slab_block = obezbedi_slab_za_typed_obj(kes, &iz_praznog_slaba);
	if (!slab_block)
		return NULL;
	void* slot = slot_alloc(slab_block, &iz_praznog_slaba);
	if (!slot)
		return NULL;
	if (kes->ctor)
		kes->ctor(slot);
	return slot;
}

void* obj_buffer_alloc(Kes* kes) // vraca obj tipa koji kes cuva
{
	Slab_block* slab_block = obezbedi_slab_za_buff_obj(kes);
	if (!slab_block)
		return NULL;
	void* slot = slab_block->header.prvi_slot;
	slab_block->header.broj_slobodnih_slotova--;
	return slot;
}

Slab_block* obrisi_objekt_u_slabu(Slab_block* slab_block, void* obj)
{
	uintptr_t prazan_slot = slab_block->header.prvi_slot;
	for (size_t i = 0; i < slab_block->header.broj_slotova; i++)
	{
		//if (!memcmp(prazan_slot, obj, slab_block->header.velicina_slota))
		if (obj == (void*)prazan_slot)
		{
			if (slab_block->header.moj_kes->dtor)
				slab_block->header.moj_kes->dtor(obj);
			memset(obj, 0, slab_block->header.velicina_slota);
			slab_block->header.broj_slobodnih_slotova++;
			return slab_block;
		}
		prazan_slot += slab_block->header.velicina_slota;
	}
	return NULL;
}

void preuredi_nepune_slabove(Kes* kes)
{
	return NULL;
	Slab_block* prvi = kes->nepun;
	Slab_block* drugi = kes->nepun->header.sledeci;
	void* slot;

	if (!drugi)
	{
		return; // ima samo jedan nepun, nema smisla premestati
		if (prvi->header.broj_slobodnih_slotova + drugi->header.broj_slobodnih_slotova > prvi->header.broj_slotova)
			return;
		for (size_t i = 0; i < drugi->header.broj_slobodnih_slotova; i++)
		{
			if (*(char*)(drugi->header.prvi_slot + i * drugi->header.velicina_slota) != 0)
			{
				/*slot = slot_alloc(prvi, 3, kes);
				if (!slot)
					printf("problem");*/
				exit(1);
			}
		}
	}
	else
	{
		prvi->header.sledeci = NULL;
		oslobodi((Buddy_block*)drugi, drugi->header.stepen_dvojke);
	}
}

Slab_block* pronadji_slab_objekta_kog_brises(Kes* kes, void* obj)
{
	Slab_block* ret = NULL;
	Slab_block* next;
	Slab_block* prev = NULL;
	next = kes->pun;
	while (next) // pretrazuje  
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
	next = kes->nepun;
	prev = NULL;
	while (next)
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

void obj_free(Kes* kes, void* obj)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return; // ne postoji kes
	//unsigned char iz_punog_slaba = 0;
	Slab_block* za_brisanje = pronadji_slab_objekta_kog_brises(kes, obj);
}

void* buff_alloc(size_t size)
{
	unsigned stepen = min_stepen_za_broj_blokova(size);
	// kontrola da li moze da stane u taj kes
	if (stepen < 5 || stepen > 17)
		return NULL;
	return obj_buffer_alloc(&slab->baferisani_kesevi[stepen - 5]);
}

void buff_free(const void* buff)
{
	unsigned char iz_punog_slaba = 0;
	Kes* kes;
	for (size_t i = 0; i < BROJ_KESEVA_ZA_BAFERE; i++)
		if (kes = pronadji_slab_objekta_kog_brises(&slab->baferisani_kesevi[i], buff))
			return;
}

Kes* skupi_kes(Kes* kes) // vraca broj oslobodjenih blokova
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return NULL;
	return kes;
}

void kes_info(Kes* kes)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return;
	printf("\n*** Cache info ***\n");
	printf("Cache nem: %s\n", kes->naziv);
	printf("Size of data : %d\n", kes->velicina);
	int broj_keseva = 1;
	Kes* next = kes;
	while (next->sledeci) {
		next = next->sledeci;
		broj_keseva++;
	}
	printf("Constructor : %p\n", kes->ctor);
	printf("Destructor : %p\n", kes->dtor);

	printf("\nFull slabs : \n");
	slab_info(kes->pun);

	printf("\nHalffull slabs : \n");
	slab_info(kes->nepun);

	printf("\nEmptry slabs : \n");
	slab_info(kes->prazan);
	printf("\n******************\n");

}

int kes_error(Kes* kes)
{
	int error = 0;
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		error = 2;

	switch (error)
	{
	case 1:
		printf("No more space");
		return 1;
	case 2:
		printf("Cache does not exsist");
		return 2;
	}
	return error;
}
