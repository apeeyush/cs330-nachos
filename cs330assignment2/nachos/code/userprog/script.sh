#!/bin/bash
`touch stats_analytics`
for i in `seq 20 100`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    `./nachos -F testfile.txt $i >> tmp`
    `'$i ' >> stats_analytics`
    `tail -n 14 tmp | head -n 1 >> stats_analytics`
done