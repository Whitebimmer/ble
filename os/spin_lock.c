




#define spin_lock_init(lock)   \
	do { \
		*(lock) = SPIN_LOCK_UNLOCKED; \
	} while(0)



void spin_lock(spinlock_t *lock)
{


}


void spin_unlock(spinlock_t *lock)
{


}


