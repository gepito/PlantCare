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
		# printf "%9.4f, %7.2f, %7.2f, %s\n", $1, $3, ar_sorted[MEDMED], $0;
	};

	if (interval > prociv){

		print $1, prociv, interval;
		prociv += 1;
	}

}
