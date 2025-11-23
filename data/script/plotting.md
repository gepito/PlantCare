# plot one column over a range

plot 'd:\asset\GitHub\gepito-PlantCare\Hardware\doc\measurement\misc\DATA-heatup.TXT' using ($13) with lines

# Resolution enhancement: add 16-th fraction centigrades

plot 'd:\asset\GitHub\gepito-PlantCare\Hardware\doc\measurement\misc\DATA-heatup.TXT' using ($13 + $14/16):xtic (4) with lines

# Add labels - the content is hour value (not readabe due to overlap)

plot 'd:\asset\GitHub\gepito-PlantCare\Hardware\doc\measurement\misc\DATA-heatup.TXT' using ($13 + $14/16):xtic (4) with lines

# Add labels - only to every hour with fixed ":00" suffix

## NOTE: gprintf must be applied as XTIC accepts strings only (from conditional expression)

plot 'DATA-heatup.TXT' using ($13 + $14/16):xtic (( ($5) < 1 ? gprintf("%3.0f:00", $4) : " " )) with lines


set terminal png size 1920, 1200

set output  "heatup.png"

