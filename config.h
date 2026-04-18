#pragma once
#include <stdint.h>

// delay
#define NSEC	100000000
#define IFACE "wlan0"
#define S(x)	((const uint8_t*)(x))

typedef uint8_t* (*block_func_t)(uint8_t* ptr);
typedef struct {
	const uint8_t* name;
	const uint8_t* color;
	block_func_t	 func;
} Block;

uint8_t* block_face(uint8_t* ptr);
uint8_t* block_load(uint8_t* ptr);
uint8_t* block_ram(uint8_t* ptr);
uint8_t* block_bat(uint8_t* ptr);
uint8_t* block_ip(uint8_t* ptr);
uint8_t* block_time(uint8_t* ptr);
uint8_t* block_fortune(uint8_t* ptr);

static const Block blocks[] = {
		{.name	= S("fortune"),
		 .color = S("#888888"),
		 .func	= block_fortune},
		{.name	= S("wlan"),
		 .color = S("#00ff00"),
		 .func	= block_ip			},
		{.name	= S("load"),
		 .color = S("#ffffff"),
		 .func	= block_load		},
		{.name	= S("bat"),
		 .color = S("#ffffff"),
		 .func	= block_bat		 },
		{.name	= S("ram"),
		 .color = S("#ffffff"),
		 .func	= block_ram		 },
		{.name	= S("time"),
		 .color = S("#ffffff"),
		 .func	= block_time		},
		{.name	= S("face"),
		 .color = S("#ff79c6"),
		 .func	= block_face		},
};

static const uint32_t NUM_BLOCKS =
		sizeof(blocks) / sizeof(blocks[0]);
