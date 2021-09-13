#include"h/lockic.h"
#include"h/Strukture.h"


void lock(HANDLE handle) {
	WaitForSingleObject(handle, INFINITE);
}

void unlock(HANDLE handle) {
	ReleaseMutex(handle);
}

void* return_and_unlock(HANDLE handle, void* ret) {
	unlock(handle, ret);
	return ret;
}


HANDLE create_mutex() {
	return CreateMutex(NULL, FALSE, "mutex");
}

