#include <assert.h>
#include <stdint.h>
#include "system.h"
#include "runtime.h"

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	char *module = argv[1];

	sys_init(argc - 1, &argv[1]);
	run_init();
	return !run_load_file(module);
}
