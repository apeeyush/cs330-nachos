#!/bin/bash
`rm -f first_part`
 `touch first_part`

echo "Batch1" >> first_part
for i in `seq 1 10`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    sed "1s/.*/$i/" Batch1.txt > testfile.txt
    
     `./nachos -F testfile.txt >> tmp`
     util_line=$(tail -n 12 tmp | head -n 1)
    B=$(echo $util_line | cut -d ' ' -f 4-)
    u_line=$(tail -n 7 tmp | head -n 1)
    C=$(echo $u_line | cut -d ' ' -f 5-)
    echo "$i    $B    $C" >> first_part
    
done

echo "Batch2" >> first_part
for i in `seq 1 10`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    sed "1s/.*/$i/" Batch2.txt > testfile.txt
    
     `./nachos -F testfile.txt >> tmp`
     util_line=$(tail -n 12 tmp | head -n 1)
    B=$(echo $util_line | cut -d ' ' -f 4-)
    u_line=$(tail -n 7 tmp | head -n 1)
    C=$(echo $u_line | cut -d ' ' -f 5-)
    echo "$i    $B    $C" >> first_part
    
done

echo "Batch3" >> first_part
for i in `seq 1 10`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    sed "1s/.*/$i/" Batch3.txt > testfile.txt
    
     `./nachos -F testfile.txt >> tmp`
     util_line=$(tail -n 12 tmp | head -n 1)
    B=$(echo $util_line | cut -d ' ' -f 4-)
    u_line=$(tail -n 7 tmp | head -n 1)
    C=$(echo $u_line | cut -d ' ' -f 5-)
    echo "$i    $B    $C" >> first_part
    
done

echo "Batch4" >> first_part
for i in `seq 1 10`;
do
    echo $i
    `rm -f tmp`
    `touch tmp`
    sed "1s/.*/$i/" Batch4.txt > testfile.txt
    
     `./nachos -F testfile.txt >> tmp`
     util_line=$(tail -n 12 tmp | head -n 1)
    B=$(echo $util_line | cut -d ' ' -f 4-)
    u_line=$(tail -n 7 tmp | head -n 1)
    C=$(echo $u_line | cut -d ' ' -f 5-)
    echo "$i    $B    $C" >> first_part
    
done