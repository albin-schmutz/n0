#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "source.h"
#include "scanner.h"

static struct SCANNER scan;

static void test_simple_tokens(void)
{
	scan_take(&scan);
	assert(scan.token == tk_amper);
	scan_take(&scan);
	assert(scan.token == tk_assign);
	scan_take(&scan);
	assert(scan.token == tk_geq);
}

static void test_comment(void)
{
	scan_take(&scan);
	assert(scan.token == tk_brack_r);
}

static void test_number(void)
{
	scan_take(&scan);
	assert(scan.token == tk_int);
	assert(scan.val == 0);
	scan_take(&scan);
	assert(scan.token == tk_int);
	assert(scan.val == 0);
	scan_take(&scan);
	assert(scan.token == tk_char);
	assert(scan.val == 1);
	scan_take(&scan);
	assert(scan.token == tk_int);
	assert(scan.val == 2147483647);
	scan_take(&scan);
	assert(scan.token == tk_int);
	assert(scan.val == 0x7fffffffffffffff);
}

static void test_keyword(void)
{
	scan_take(&scan);
	assert(scan.token == kw_if);
	scan_take(&scan);
	assert(scan.token == kw_procedure);
}

static void test_identifier(void)
{
	scan_take(&scan);
	assert(scan.token == tk_ident);
	assert(!strcmp("if", scan.ident));
	scan_take(&scan);
	assert(scan.token == tk_ident);
	assert(!strcmp("procedure", scan.ident));
	scan_take(&scan);
	assert(scan.token == tk_ident);
	assert(!strcmp("A00", scan.ident));
}

int main(void)
{
	strcpy(scan.modulename, "scanner_test");
	scan_create(&scan, &scan);

	test_simple_tokens();
	test_comment();
	test_number();
	test_keyword();
	test_identifier();

	return 0;
}
