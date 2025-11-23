Column	Content
[1-6]	timestamp: yy MM dd hh ss mm 
7		nLoop: increment after startup
8		nDry: dry condition sensed, pump when nDry>5
9		nPump: number of pump activations
[10-12]	sensor H/L/Moisture
	// moisture below MOIST_LOW consideered dry
	const unsigned int MOIST_LOW = 675;

    if (sensorMoisture > MOIST_LOW) {
      // totally dry reads 1023, totally wet ~300
      // may be confusing to call it moisture as reading is inversly proportional
13		temperature: fractional part is in 1/16 C 
