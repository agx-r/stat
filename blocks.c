#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#include "blocks.h"
#include "config.h"

typedef struct {
	const uint8_t* word;
	uint8_t				 wlen;
	const uint8_t* rep;
	uint8_t				 rlen;
} DictWord;

static const DictWord dict[] = {
		W("woman", "grl"),
		W("girl", "grl"),
		W("mathematic", "math"),
		W("ped", "pd"),
		W("they", "thy"),
		W("when", "whn"),
		W("tten", "tn"),
		W("using", "usng"),
		W("because", "cuz"),
		W("without", "w/o"),
		W("something", "smth"),
		W("fucking", "fkn"),
		W("fuck", "fck"),
		W("shit", "sht"),
		W("windows", "win"),
		W("linux", "lnx"),
		W("administrator", "adm"),
		W("you", "u"),
		W("are", "r"),
		W("and", "&")};
static const uint8_t dict_size =
		sizeof(dict) / sizeof(dict[0]);

[[gnu::nonstring]]
static const uint8_t digits_lut[200] =
		"0001020304050607080910111213141516171819"
		"2021222324252627282930313233343536373839"
		"4041424344454647484950515253545556575859"
		"6061626364656667686970717273747576777879"
		"808182838485868788899091929394959697989"
		"9";

static inline uint8_t*
write_digits_fast(uint8_t* p, uint32_t val) {
	memcpy(p, &digits_lut[val * 2], 2);
	return p + 2;
}

static inline uint8_t*
write_u32_fast(uint8_t* p, uint32_t val) {
	if (val == 0) {
		*p++ = '0';
		return p;
	}
	uint8_t	 buf[10];
	uint8_t* start = buf + 10;
	while (val >= 100) {
		start -= 2;
		memcpy(start, &digits_lut[(val % 100) * 2],
					 2);
		val /= 100;
	}
	if (val < 10) *--start = (uint8_t)('0' + val);
	else {
		start -= 2;
		memcpy(start, &digits_lut[val * 2], 2);
	}
	uint8_t len = (uint8_t)((buf + 10) - start);
	memcpy(p, start, len);
	return p + len;
}

void
append_str(uint8_t** restrict p,
					 const uint8_t* restrict str) {
	uint16_t len = 0;
	while (str[len]) len++;
	memcpy(*p, str, len);
	*p += len;
}

static inline int8_t
parse_cap(const uint8_t* s) {
	int8_t res = 0;
	while (*s >= '0' && *s <= '9') {
		res = res * 10 + (int8_t)(*s - '0');
		s++;
	}
	return res;
}

static inline bool
streq_u8(const void* a, const uint8_t* b) {
	const uint8_t* s1 = (const uint8_t*)a;
	while (*s1 && *s1 == *b) {
		s1++;
		b++;
	}
	return *s1 == *b;
}

uint8_t*
block_face(uint8_t* ptr) {
	static const uint8_t* const faces[] = {
			S("( >.<) "), S("( -.-) "),	 S("(^ω^) "),
			S("(◕‿◕) "),	S("(UwU) "),	 S("( ¬_¬) "),
			S("(≖‿≖ )"),	S("(=^ω^=) "), S("(◕.◕) "),
			S("(•‿•) "),	S("(UwU) ")};

	static uint32_t seed = 0;
	if (seed == 0) seed = (uint32_t)time((void*)0);

	uint8_t idx =
			(uint8_t)((seed + (uintptr_t)ptr) %
								(uint8_t)(sizeof(faces) /
													sizeof(faces[0])));
	append_str(&ptr, faces[idx]);
	return ptr;
}

uint8_t*
block_load(uint8_t* ptr) {
	append_str(&ptr, S("LOAD "));
	static int16_t fd = -1;
	if (fd < 0) {
		fd = (int16_t)open("/proc/loadavg",
											 O_RDONLY | O_CLOEXEC);
		if (fd < 0) return ptr;
	} else lseek(fd, 0, SEEK_SET);

	uint8_t buf[16];
	int8_t	n = (int8_t)read(fd, buf, 15);
	if (n > 0) {
		void* space_pos =
				memchr(buf, ' ', (uint8_t)n);
		uint8_t len =
				space_pos
						? (uint8_t)((uint8_t*)space_pos - buf)
						: (uint8_t)n;
		memcpy(ptr, buf, len);
		ptr += len;
	}
	return ptr;
}

uint8_t*
block_ram(uint8_t* ptr) {
	append_str(&ptr, S("RAM "));
	struct sysinfo si;
	if (sysinfo(&si) < 0) return ptr;
	uint32_t used_mb =
			(uint32_t)(((si.totalram - si.freeram -
									 si.bufferram) *
									si.mem_unit) >>
								 20);
	ptr		 = write_u32_fast(ptr, used_mb);
	*ptr++ = 'M';
	return ptr;
}

uint8_t*
block_bat(uint8_t* ptr) {
	append_str(&ptr, S("BAT "));
	static int16_t fd = -1;
	if (fd < 0) {
		fd = (int16_t)open(
				"/sys/class/power_supply/BAT0/capacity",
				O_RDONLY | O_CLOEXEC);
		if (fd < 0) return ptr;
	} else lseek(fd, 0, SEEK_SET);

	uint8_t buf[8];
	int8_t	n = (int8_t)read(fd, buf, sizeof(buf));
	if (n > 0) {
		int8_t cap = parse_cap(buf);
		if (cap == 100) append_str(&ptr, S("1.0"));
		else {
			append_str(&ptr, S("0."));
			if (cap < 10) *ptr++ = '0';
			ptr = write_u32_fast(ptr, (uint32_t)cap);
		}
	}
	return ptr;
}

uint8_t*
block_ip(uint8_t* ptr) {
	append_str(&ptr, S("NET "));
	struct ifaddrs *ifaddr, *ifa;
	bool						found							 = false;
	uint8_t addr_res[INET6_ADDRSTRLEN] = {0};

	if (getifaddrs(&ifaddr) == -1) {
		append_str(&ptr, S("down"));
		return ptr;
	}

	for (ifa = ifaddr; ifa != NULL;
			 ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
				ifa->ifa_addr->sa_family == AF_INET6) {
			if (streq_u8(ifa->ifa_name, S(IFACE))) {
				struct sockaddr_in6* s6 =
						(struct sockaddr_in6*)ifa->ifa_addr;
				if (IN6_IS_ADDR_LOOPBACK(&s6->sin6_addr))
					continue;
				inet_ntop(AF_INET6, &s6->sin6_addr,
									(void*)addr_res,
									sizeof(addr_res));
				found = true;
				if (!IN6_IS_ADDR_LINKLOCAL(
								&s6->sin6_addr))
					break;
			}
		}
	}

	if (found) append_str(&ptr, addr_res);
	else append_str(&ptr, S("down"));

	freeifaddrs(ifaddr);
	return ptr;
}

uint8_t*
block_time(uint8_t* ptr) {
	append_str(&ptr, S("TIME "));
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	struct tm tm_info;
	localtime_r(&ts.tv_sec, &tm_info);

	uint16_t year =
			(uint16_t)(1900 + tm_info.tm_year);
	ptr = write_digits_fast(ptr,
													(uint32_t)(year / 100));
	ptr = write_digits_fast(ptr,
													(uint32_t)(year % 100));
	*ptr++ = ':';

	ptr = write_digits_fast(
			ptr, (uint32_t)(tm_info.tm_mon + 1));
	*ptr++ = ':';

	ptr = write_digits_fast(
			ptr, (uint32_t)tm_info.tm_mday);
	*ptr++ = ':';

	ptr = write_digits_fast(
			ptr, (uint32_t)tm_info.tm_hour);
	*ptr++ = ':';
	ptr		 = write_digits_fast(
			ptr, (uint32_t)tm_info.tm_min);
	*ptr++ = ':';
	ptr		 = write_digits_fast(
			ptr, (uint32_t)tm_info.tm_sec);
	*ptr++ = ':';

	uint16_t ms = (uint16_t)(ts.tv_nsec / 1000000);
	*ptr++			= (uint8_t)('0' + (ms / 100));
	ptr = write_digits_fast(ptr,
													(uint32_t)(ms % 100));

	return ptr;
}

uint8_t*
block_fortune(uint8_t* ptr) {
	static uint8_t	quote_buf[512] = {0};
	static uint16_t quote_len			 = 0;
	static uint32_t last_update		 = 0;

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (last_update == 0 ||
			(ts.tv_sec - last_update) >= 30) {
		static int16_t	fd				= -1;
		static uint8_t* file_map	= (uint8_t*)-1;
		static uint32_t file_size = 0;

		if (fd < 0) {
			fd = (int16_t)open(
					"/usr/share/fortune/fortunes2-o",
					O_RDONLY | O_CLOEXEC);
			if (fd >= 0) {
				file_size =
						(uint32_t)lseek(fd, 0, SEEK_END);
				file_map = (uint8_t*)mmap(
						NULL, file_size, PROT_READ,
						MAP_PRIVATE, fd, 0);
				if (file_map == MAP_FAILED)
					file_map = (uint8_t*)-1;
			}
		}

		if (file_map != (uint8_t*)-1 &&
				file_size > 0) {
			static uint32_t seed = 0;
			if (seed == 0)
				seed = (uint32_t)ts.tv_nsec | 1;

			bool found = false;
			for (uint8_t attempt = 0;
					 attempt < 15 && !found; ++attempt) {
				seed ^= seed << 13;
				seed ^= seed >> 17;
				seed ^= seed << 5;

				uint32_t pos = seed % file_size;
				while (pos > 0 && file_map[pos] != '%')
					pos--;
				if (file_map[pos] == '%') pos++;

				uint8_t	 tmp_buf[256];
				uint16_t tmp_len = 0;

				while (pos < file_size &&
							 file_map[pos] != '%' &&
							 tmp_len < sizeof(tmp_buf)) {
					uint8_t c = file_map[pos++];

					if (c == '\'') continue;

					if (c >= 'A' && c <= 'Z') c += 32;

					if (c == '\n' || c == '\r' || c == '\t')
						c = ' ';

					if (c == ' ') {
						if (tmp_len == 0) continue;
						uint8_t prev = tmp_buf[tmp_len - 1];
						if (prev == ' ' || prev == '.' ||
								prev == ',')
							continue;
					}

					if (c == '.') {
						if (tmp_len >= 2 &&
								tmp_buf[tmp_len - 1] == '.' &&
								tmp_buf[tmp_len - 2] == '.') {
							continue;
						}
					}

					if (c >= 32) tmp_buf[tmp_len++] = c;
				}

				while (tmp_len > 0 &&
							 tmp_buf[tmp_len - 1] == ' ')
					tmp_len--;

				if (tmp_len < 20 || tmp_len > 80)
					continue;

				uint8_t	 rep_buf[256];
				uint16_t rep_len = 0;

				for (uint16_t i = 0; i < tmp_len;) {
					bool matched = false;
					for (uint8_t j = 0; j < dict_size;
							 ++j) {
						if (i + dict[j].wlen <= tmp_len &&
								memcmp(&tmp_buf[i], dict[j].word,
											 dict[j].wlen) == 0) {

							memcpy(&rep_buf[rep_len],
										 dict[j].rep, dict[j].rlen);
							rep_len += dict[j].rlen;
							i += dict[j].wlen;
							matched = true;
							break;
						}
					}
					if (!matched)
						rep_buf[rep_len++] = tmp_buf[i++];
				}

				quote_len = 0;
				for (uint16_t i = 0; i < rep_len; ++i) {
					uint8_t c = rep_buf[i];
					if (c == '"' || c == '\\') {
						quote_buf[quote_len++] = '\\';
					}
					quote_buf[quote_len++] = c;
				}

				last_update = (uint32_t)ts.tv_sec;
				found				= true;
			}

			if (!found) last_update = 0;
		}
	}

	if (quote_len > 0) {
		memcpy(ptr, quote_buf, quote_len);
		ptr += quote_len;
	}

	return ptr;
}
