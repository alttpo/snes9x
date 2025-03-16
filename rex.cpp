
#ifdef USE_REX

extern "C" {
#include "trex.h"
}

struct trex_context rexctx;
uint32_t rexstack[32];

void S9xRexInit(void) {
	// initialize trex context to execute state machines with:
	trex_context_init(&rexctx, nullptr, rexstack, 32);

	//rexctx.machines_count;
}

void S9xRexExec(void) {
	trex_exec(&rexctx, 25);
}

#endif
