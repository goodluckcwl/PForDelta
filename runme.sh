small_index_fraction=0.90
num_bins=10000
max_bin_size=10000
uniform_int_low=100
uniform_int_high=200000

for (( mean = 50; mean <= 200; mean+=10)) do
	./unittest $mean $small_index_fraction $num_bins $max_bin_size $uniform_int_low $uniform_int_high 
done
