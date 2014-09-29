#!/bin/bash
`touch stats_analytics`
for i in `seq 20 100`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    `./nachos -F testfile.txt $i >> tmp`
    util_line=$(tail -n 12 tmp | head -n 1)
    B=$(echo $util_line | cut -d ' ' -f 4-)
    echo "$i $B" >> stats_analytics
done