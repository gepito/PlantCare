# Calculate average current for intervals 1::5
# The average is calculated after median filtering

BEGIN {
	FS = ",";
	gotiv = 0;	# has already entered target interval
	insix = 0;	# insertion index

#	MEDLEN = 7;	# length of median filtering
#	MEDMED = 4;	# the index of median element. The resulting indices are in [1..MEDLEN] range

	MEDLEN = 11;	# length of median filtering
	MEDMED = 6;	# the index of median element. The resulting indices are in [1..MEDLEN] range

	prociv = 0;	# the interval to be processed
	dev2_sum = 0;
	iv_n = 0;
	
	for (i = 0; i < ARGC; i++)
		if ( i >= 2) {
		
			# expect 5 average values for 5 measurement stages as parameter after file name
			# delete averages to prevent processing them as file names
			
			# printf "AVG[%d] : %f\n", i-2, ARGV[i];
			
			avg[i-2] = ARGV[i];
			delete ARGV[i];
		}
}

{
	
	interval = $4 + 2*$5 + 4*$6;
	if (($1 > 0.2) && (interval == prociv)) {
	
		# apply median filter of MEDLEN length
		
		ar[insix] = $3;
		insix += 1;
		if (insix >= MEDLEN){ insix = 0;}

		asort(ar, ar_sorted);

		dev2_sum += (ar_sorted[MEDMED] - avg[interval]) * (ar_sorted[MEDMED] - avg[interval]);
		iv_n += 1;

		# print iv_n, ar_sorted[MEDMED], avg[interval], dev2_sum, sqrt(dev2_sum/iv_n);
		
	};

	if (interval > prociv){
		if (iv_n > 0) {
			ar_dev[prociv] = sqrt(dev2_sum / iv_n);
			# print interval, dev2_sum, iv_n, ar_dev[prociv];
		}
		else {
			ar_dev[prociv] = 0;
			printf "Error suspicion: interval %d is empty!?\n", interval;
		}

		prociv += 1;
		dev2_sum = 0;
		iv_n = 0;

		if (prociv == 5) {	# terminate process after 5 intervals has been processed
			printf "%s", FILENAME;
			for (i=0; i<5; i++){printf ", %7.2f", ar_dev[i];}
			print "";
			exit;
		}
	}

}
