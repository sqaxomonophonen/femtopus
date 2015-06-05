#ifndef SCRATCH_H

#include <stdlib.h>
#include <string.h>

#include "a.h"

struct scratch {
	size_t increment;
	void* first;

	// state
	void* current;
	size_t used_in_current;
};

inline static void scratch_reset(struct scratch* s)
{
	s->current = s->first;
	s->used_in_current = sizeof(void*);
}

inline static void scratch_init(struct scratch* s, size_t increment)
{
	AN(s);
	ASSERT(increment > sizeof(void*));
	memset(s, 0, sizeof(*s));
	s->increment = increment;
	AN(s->first = malloc(increment));
	void** next = (void**)s->current;
	*next = NULL;
	scratch_reset(s);
}

inline static void scratch_assert_valid(struct scratch* s)
{
	AN(s);
	AN(s->first); // fails if freed
}

inline static void scratch_free(struct scratch* s)
{
	scratch_assert_valid(s);

	void* current;
	void* next;

	current = s->first;
	while (current) {
		next = *((void**)current);
		free(current);
		current = next;
	}

	memset(s, 0, sizeof(*s));
}

struct scratch_savepoint {
	void* current;
	size_t used_in_current;
};

inline static struct scratch_savepoint scratch_save(struct scratch* s)
{
	scratch_assert_valid(s);

	struct scratch_savepoint sp;
	sp.current = s->current;
	sp.used_in_current = s->used_in_current;
	return sp;
}

inline static void scratch_recall(struct scratch* s, struct scratch_savepoint sp)
{
	scratch_assert_valid(s);
	s->current = sp.current;
	s->used_in_current = sp.used_in_current;
}

inline static void* scratch_alloc_aligned_log2(struct scratch* s, size_t sz, int alignment_log2)
{
	scratch_assert_valid(s);

	int alignment = 1 << alignment_log2;
	int mask = alignment - 1;
	int again;
	for (again = 0; again < 2; again++) {
		// align allocation
		int off = s->used_in_current & mask;
		if (off != 0) s->used_in_current += alignment - off;

		// save offset
		size_t offset = s->used_in_current;

		// add allocation to used
		s->used_in_current += sz;

		// check we're within bounds
		if (s->used_in_current > s->increment)  {
			// second try should always succeed, otherwise throw up
			if (again) {
				arghf(
					"allocation of %zd bytes with alignment %d "
					"does not fit in scratch allocator "
					"with %zd increment",
					sz, alignment, s->increment);
			}

			// allocate new block if end of linked list
			void** next = (void**)s->current;
			if (*next == NULL) *next = malloc(s->increment);
			AN(s->current = *next);
			s->used_in_current = sizeof(void*);

			// try again, this time allocate or bust
			continue;
		}

		// return allocated memory
		return (void*) ((uint8_t)s->current + offset);
	}
	arghf("unreachable");
}

inline static void* scratch_alloc_a1(struct scratch* s, size_t sz)
{
	return scratch_alloc_aligned_log2(s, sz, 0);
}

inline static void* scratch_alloc_a2(struct scratch* s, size_t sz)
{
	return scratch_alloc_aligned_log2(s, sz, 1);
}

inline static void* scratch_alloc_a4(struct scratch* s, size_t sz)
{
	return scratch_alloc_aligned_log2(s, sz, 2);
}

inline static void* scratch_alloc_a8(struct scratch* s, size_t sz)
{
	return scratch_alloc_aligned_log2(s, sz, 3);
}

inline static void* scratch_alloc_a16(struct scratch* s, size_t sz)
{
	return scratch_alloc_aligned_log2(s, sz, 4);
}

inline static void* scratch_alloc(struct scratch* s, size_t sz)
{
	return scratch_alloc_a8(s, sz);
}

#define SCRATCH_H
#endif
