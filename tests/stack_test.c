/* PC test of jq's exec_stack.h with the 68k's 2-byte alignment (built with
 * -fpack-struct=2, so offsetof() gives 2 as on m68k): blocks pushed and
 * popped keep their contents and their "next" links (patches/0002).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "exec_stack.h"

/* jv_alloc.c's, without the rest of jq */
void *jv_mem_realloc(void *p, size_t sz) { return realloc(p, sz); }

static int fails, checks;
static void ok(int c, const char *what) { checks++; if (!c) { fails++; printf("FAIL %s\n", what); } }

int main(void)
{
	struct stack s;
	stack_ptr p[64];
	int i, j, good = 1;
	stack_init(&s);
	printf("natural alignment here: %d, stack ALIGNMENT: %d\n",
	       (int)offsetof(struct determine_alignment, u), (int)ALIGNMENT);
	ok(ALIGNMENT >= (int)sizeof(stack_ptr), "ALIGNMENT holds a stack_ptr");
	/* 16-byte blocks (jq's jv), each linked to the one before, filled with 0xA0+i */
	for (i = 0; i < 64; i++) {
		p[i] = stack_push_block(&s, i ? p[i - 1] : 0, 16);
		memset(stack_block(&s, p[i]), 0xA0 + i, 16);
	}
	for (i = 63; i >= 0; i--) {
		unsigned char *b = stack_block(&s, p[i]);
		for (j = 0; j < 16; j++) if (b[j] != (unsigned char)(0xA0 + i)) good = 0;
		ok(*stack_block_next(&s, p[i]) == (i ? p[i - 1] : 0), "next link kept");
		ok(stack_pop_block(&s, p[i], 16) == (i ? p[i - 1] : 0), "pop returns next");
	}
	ok(good, "block contents kept");
	ok(s.limit == 0, "stack empty after popping all");
	stack_reset(&s);
	printf("stack: %d of %d checks passed\n", checks - fails, checks);
	return fails != 0;
}
