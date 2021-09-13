#include <string.h>
#include "../h/slab.h"
#include "../h/Strukture.h"
#include "../h/lockic.h"

#define BLOCK_SIZE (4096)

int index_tipskog_kesa(Kes* kes)
{
	lock(slab->mutex);
	for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
		if (kes == slab->ulancani_kesevi + i) {
			if(slab->niz_napravljenih_keseva[i])
				return return_and_unlock(slab->mutex, i);
			return return_and_unlock(slab->mutex, -1);
		}
	//return i;
//return -1;
	return return_and_unlock(slab->mutex, -1);
}

void inic_baferske_keseve()
{
	char buffer[15];
	for (size_t i = 0; i < 13; i++)
	{
		memset(slab->baferisani_kesevi + i, 0, sizeof(Kes));
		sprintf_s(buffer, 15, "Mem Buffer %d", i + 5);
		slab->baferisani_kesevi[i].naziv = buffer;
		slab->baferisani_kesevi[i].velicina = pow(2, i + 5);
		slab->baferisani_kesevi[i].mutex = create_mutex();
	}
}

void inic_tipske_keseve() {
	memset(slab->ulancani_kesevi, 0, sizeof(Kes) * BROJ_TIPSKIH_KESEVA);
}

void slab_inic(uintptr_t pocetna_adresa, unsigned ukupan_broj_blokova)
{
	slab = (Slab*)pocetna_adresa;
	inic_baferske_keseve();
	slab->mutex = create_mutex();

	slab->ulancani_kesevi = (Kes*)(pocetna_adresa + sizeof(Slab)); // prazna lista keseva
	inic_tipske_keseve();
	memset(slab->niz_napravljenih_keseva, 0, BROJ_TIPSKIH_KESEVA * sizeof(unsigned char));

	//unsigned broj_bajtova = sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(Kes);
	slab->buddy.broj_blokova = (unsigned)(ukupan_broj_blokova - (unsigned)ceil(((sizeof(Slab) + BROJ_TIPSKIH_KESEVA * sizeof(Kes)) * 1.) / BLOCK_SIZE)); // ova linija moze da bude promeljiva

	slab->buddy.pocetna_adesa = (Buddy*)(pocetna_adresa + (uintptr_t)((ukupan_broj_blokova - slab->buddy.broj_blokova) * BLOCK_SIZE));

	buddy = &slab->buddy;
	memset(buddy->niz_slobodnih_blokova, 0, VELICINA_PAMTLJIVOG * sizeof(Buddy_block*));
	buddy_inic();
}

Kes* daj_prazno_mesto_za_kes()
{
	lock(slab->mutex);
	for (size_t i = 0; i < BROJ_TIPSKIH_KESEVA; i++)
		if (!slab->niz_napravljenih_keseva[i])
		{
			slab->niz_napravljenih_keseva[i] = 1;
			return return_and_unlock(slab->mutex, slab->ulancani_kesevi + i);
		}
	return return_and_unlock(slab->mutex, NULL);
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
	kes->mutex = create_mutex();

	lock(kes->mutex);
	kes->prazan = slab_alloc_typed(kes);
	if (!kes->prazan)
		return return_and_unlock(kes->mutex, NULL);
	//return NULL;

//return kes;
	return return_and_unlock(kes->mutex, kes);
}

void oslobodi_slabove_kesa(Kes* kes)
{
	lock(buddy->mutex);
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
	kes->prazan = NULL;
	kes->nepun = NULL;
	kes->pun = NULL;
	unlock(buddy->mutex);
}

void kes_free(Kes* kes)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return NULL;
	oslobodi_slabove_kesa(kes);
	lock(slab->mutex);
	slab->niz_napravljenih_keseva[index] = 0;
	memset(slab->ulancani_kesevi + index, 0, sizeof(Kes));
	unlock(slab->mutex);
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
		return NULL; // greska 
	unsigned char iz_praznog_slaba = 1;
	lock(kes->mutex);
	Slab_block* slab_block = obezbedi_slab_za_typed_obj(kes, &iz_praznog_slaba);
	if (!slab_block)
		//return NULL;
		return return_and_unlock(kes->mutex, NULL);
	void* slot = slot_alloc(slab_block, &iz_praznog_slaba);
	if (!slot)
		//return NULL;
		return return_and_unlock(kes->mutex, NULL);
	if (kes->ctor)
		kes->ctor(slot);
	//return slot;
	return return_and_unlock(kes->mutex, slot);
}

void* obj_buffer_alloc(Kes* kes) // vraca obj tipa koji kes cuva
{
	lock(kes->mutex);
	Slab_block* slab_block = obezbedi_slab_za_buff_obj(kes);
	if (!slab_block)
		//return NULL;
		return return_and_unlock(kes->mutex, NULL);
	short iz_praznog_slaba = 1;
	void* slot = slot_alloc(slab_block, &iz_praznog_slaba);
	if (!slot)
		//return NULL;
		return return_and_unlock(kes->mutex, NULL);
	//return slot;
	return return_and_unlock(kes->mutex, slot);
}

Slab_block* obrisi_objekt_u_slabu(Slab_block* slab_block, void* obj)
{
	uintptr_t prazan_slot = slab_block->header.prvi_slot;
	for (size_t i = 0; i < slab_block->header.broj_slotova; i++)
	{
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

Slab_block* pronadji_slab_objekta_kog_brises(Kes* kes, void* obj)
{
	Slab_block* ret = NULL;
	Slab_block* prev = NULL;
	Slab_block* next = kes->pun;
	while (next)
	{ // pretrazuje pune
		if (ret = obrisi_objekt_u_slabu(next, obj))
		{ // nasao slab objekta kojeg izbacuje
			if (prev) // pun slab ima prethodnika u listi
				prev->header.sledeci = ret->header.sledeci; // izbacen iz liste punih
			else // pun slab nema prethnodnika u listi
				kes->pun = ret->header.sledeci; // izbacen iz liste punih
			if (ret->header.broj_slobodnih_slotova == ret->header.broj_slotova)
			{ // prazan je, treba ga staviti u prazne slabove
				if (kes->prazan)
					ret->header.sledeci = kes->prazan;
				kes->prazan = ret;
				return ret;
			}
			else
			{ // nije prazan, treba ga ubaciti po sortu u nepune
				prev = NULL;
				if (!kes->nepun)
					kes->nepun = ret;
				else {
					next = kes->nepun;
					while (next) {
						if (ret->header.broj_slobodnih_slotova > next->header.broj_slobodnih_slotova)
							break;
						prev = next;
						next = next->header.sledeci;
					}
					ret->header.sledeci = next;
					prev->header.sledeci = ret;
				
				}
				return ret;
			}
		}
		prev = next;
		next = next->header.sledeci;
	}


	next = kes->nepun;
	prev = NULL;
	while (next)
	{ // pretrazuje nepune
		if (ret = obrisi_objekt_u_slabu(next, obj))
			// nasao slab objekta kojeg izbacuje
			if (ret->header.broj_slobodnih_slotova == ret->header.broj_slotova)
			{ // prazan je, treba ga staviti u prazne slabove
				if (!prev)
				{ // nema prethondika u nepunim
					kes->nepun = kes->nepun->header.sledeci;

					if (!kes->prazan) // nema ni jedan prazan slab
						kes->prazan = ret;
					else
					{ // ima makar jedan prazan slab
						ret = kes->prazan->header.sledeci;
						kes->prazan->header.sledeci = ret;
					}
				}
				return ret;
			}
			else
			{ // nije prazan, treba proveriti da li nepuni ostaju sortirani
				if (prev) {
					if (prev->header.broj_slobodnih_slotova < ret->header.broj_slobodnih_slotova)
					{ // mora da se sortira, prethodni ima vise zauzetih
						next = kes->nepun;
						Slab_block* t = ret->header.sledeci;
						if (prev = next)
						{ // prev nema prethodnika
							ret->header.sledeci = prev;
							prev->header.sledeci = t;
							kes->nepun = ret;
						}
						else
						{ // prev ima prethodnike
							while (next->header.sledeci != prev)
								next = next->header.sledeci;
							prev->header.sledeci = t;
							ret->header.sledeci = prev;
							next->header.sledeci = ret;
						}
					}
				}
				return ret;
			}
		prev = next;
		next = next->header.sledeci;
	}
	return NULL;
}

void obj_free(Kes* kes, void* obj)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return;
	lock(kes->mutex);
	Slab_block* za_brisanje = pronadji_slab_objekta_kog_brises(kes, obj);
	unlock(kes->mutex);
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
	lock(slab->mutex);
	for (size_t i = 0; i < BROJ_KESEVA_ZA_BAFERE; i++) {
		if (kes = pronadji_slab_objekta_kog_brises(&slab->baferisani_kesevi[i], buff)) {
			unlock(slab->mutex);
			return;
		}
	}
}

int skupi_kes(Kes* kes) // vraca broj oslobodjenih blokova
{
	int osobodjeni_kesevi = 0;
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return NULL;

	lock(kes->mutex);

	int pola = kes->velicina;
	Slab_block* next = kes->nepun;
	Slab_block* prev = NULL;
	while (next) {
		//if ()

	}

	next = kes->prazan;
	prev = NULL;
	while (next) {
		prev = next;
		next = next->header.sledeci;
	}
	if (!next)
		return osobodjeni_kesevi;
	else {
		if (!prev) {
			return osobodjeni_kesevi + bajtove_u_min_blokova(kes->velicina);
		}
		else {

		}

	}
	//return osobodjeni_kesevi;
	return return_and_unlock(kes->mutex, osobodjeni_kesevi);
}

void kes_info(Kes* kes)
{
	int index = index_tipskog_kesa(kes);
	if (index == -1)
		return;
	printf("\n*** Cache info ***\n");
	printf("Cache nem: %s\n", kes->naziv);
	printf("Size of data : %d\n", kes->velicina);
	/*int broj_keseva = 1;
	Kes* next = kes;
	while (next->sledeci) {
		next = next->sledeci;
		broj_keseva++;
	}*/
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
