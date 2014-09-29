#!/bin/bash
`rm -f second_part`
 `touch second_part`

echo "Batch5" >> second_part
for i in `seq 1 2`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    sed "1s/.*/$i/" Batch5.txt > testfile.txt
    
     `./nachos -F testfile.txt >> tmp`
    u_line=$(tail -n 7 tmp | head -n 1)
    C=$(echo $u_line | cut -d ' ' -f 5-)
    echo "$i    $C" >> second_part
    
done