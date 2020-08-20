# Calculate average current for intervals 1::5
# The average is calculated after median filtering

BEGIN {
	FS = ",";
	gotiv = 0;	# has already entered target interval
	insix = 0;	# insertion index
	MEDLEN = 7;	# length of median filtering
	MEDMED = 4;	# the index of median element. The resulting indices are in [1..MEDLEN] range
	prociv = 0;	# the interval to be processed
	iv_sum = 0;
	iv_n = 0;
}

{
	
	interval = $4 + 2*$5 + 4*$6;
	if (($1 > 0.2) && (interval == prociv)) {
		ar[insix] = $3;
		insix += 1;
		if (insix >= MEDLEN){ insix = 0;}

		asort(ar, ar_sorted);

		iv_sum += ar_sorted[MEDMED];
		iv_n += 1;
	};

	if (interval > prociv){
		if (iv_n > 0) {ar_av[prociv] = iv_sum / iv_n;}
		else {ar_av[prociv] = 0;}

		#print "p_", prociv, iv_n, iv_sum, iv_sum / iv_n;

		prociv += 1;
		iv_n = 1;
		iv_sum = ar_sorted[MEDMED];

		if (prociv == 5) {	# terminate process after 5 intervals has been processed
			printf "%s", FILENAME;
			for (i=0; i<5; i++){printf ", %7.2f", ar_av[i];}
			print "";
			exit;
		}
	}

}
