BEGIN {
	FS = ",";
	gotiv = 0;	# has already entered target interval
	insix = 0;	# insertion index
	MEDLEN = 7;	# length of median filtering
	MEDMED = 4;	# the index of median element. The resulting indices are in [1..MEDLEN] range
}

{
	
	interval = $4 + 2*$5 + 4*$6;
	if (interval == 1) {
		ar[insix] = $3;
		insix += 1;
		if (insix >= MEDLEN){ insix = 0;}

		asort(ar, ar_sorted);

		print "in:", $0;
		print "   ", $1,$3, ar_sorted[MEDMED];
		print "> >", insix;
		for (x in ar) {print x, ar[x];}
		print ">S>";
		for (x in ar_sorted) {print x, ar_sorted[x];}

		gotiv = 1;
	}
	else {
		if (gotiv == 1) {exit;}
	}
}
