stime(tp)
	long *tp;
{
	struct { long l1,l2; } x;

	x.l1 = *tp;
	x.l2 = 0;
	settimeofday(&x, (char *) 0);
}
