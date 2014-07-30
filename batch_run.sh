rm output
DATA=/intrepid-fs0/users/sriraml/scratch/alacrity/extract/small

for ((i = 0; i < 1000; i++)) do

  echo "Block " $i
  ./unittest saurabh ./data/blocks/temp $i | tee -a output
done

rm comp_ratios comp_throughput decomp_throughput
grep 'Compression ratio' output | awk '{print $4}' >> comp_ratios
grep 'Avg. compression throughput' output | awk '{print $5}' >> comp_throughput
grep 'Avg. decompression throughput' output | awk '{print $5}' >> decomp_throughput
