BEGIN {

	print ARGC;

	for (i = 0; i < ARGC; i++)
		if ( i >= 2) {
			printf "AVG[%d] : %f\n", i-2, ARGV[i];
			avg[i-1] = ARGV[i];
		}

	for (i = 0; i < ARGC; i++)
		print i, ARGV[i];
	for (i = 2; i < ARGC; i++)
		delete ARGV[i];
	};
{
	if (NR < 5) print $0;
}