void _start(void)
{
	*(unsigned long long*)0xb8000 = 0x4141414141414141ULL;

	for (volatile int i = 0; i == 0;) {}
}