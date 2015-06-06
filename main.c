#include <stdio.h>

#include "llvl.h"

int main(int argc, char** argv)
{
	struct lvl lvl;
	llvl_build("thing", &lvl);
	lvl_free(&lvl);
	return 0;
}
