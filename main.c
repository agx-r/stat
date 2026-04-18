#include <time.h>
#include <unistd.h>

#include "config.h"
#include "blocks.h"

int
main(void) {
	write(STDOUT_FILENO, S("{\"version\":1}\n[\n"),
				16);
	alignas(64) uint8_t buf[2048];
	const uint8_t*			prefix = S("[");
	struct timespec			ts		 = {.tv_sec	 = 0,
																.tv_nsec = NSEC};

	while (true) {
		uint8_t* p = buf;
		append_str(&p, prefix);
		prefix = S(",[");

		for (uint8_t i = 0; i < NUM_BLOCKS; ++i) {
			if (i > 0) append_str(&p, S(","));
			append_str(&p, S("{\"name\":\""));
			append_str(&p, blocks[i].name);
			append_str(&p, S("\",\"color\":\""));
			append_str(&p, blocks[i].color);
			append_str(&p, S("\",\"full_text\":\""));
			p = blocks[i].func(p);
			append_str(&p, S("\"}"));
		}
		append_str(&p, S("]\n"));
		write(STDOUT_FILENO, buf,
					(uint16_t)(p - buf));
		nanosleep(&ts, NULL);
	}
}
