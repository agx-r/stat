#pragma once
#include <stdint.h>

#define W(w, r)                                  \
	{S(w), sizeof(w) - 1, S(r), sizeof(r) - 1}

void append_str(uint8_t** restrict p,
					 const uint8_t* restrict str);
