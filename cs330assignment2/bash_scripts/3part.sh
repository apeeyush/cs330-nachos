`rm -f third_part`
`rm -f tmp`
`touch tmp`
echo "Batch1" >> third_part
sed "1s/.*/2/" Batch1.txt > testfile.txt
`./nachos -F testfile.txt >> tmp`
u_line=$(tail -n 15 tmp | head -n 1)
A=$(echo $u_line | cut -d ' ' -f 5-)
echo "$i  $A" >> third_part
`rm -f tmp`
`touch tmp`
echo "Batch2" >> third_part
sed "1s/.*/2/" Batch2.txt > testfile.txt
`./nachos -F testfile.txt >> tmp`
u_line=$(tail -n 15 tmp | head -n 1)
A=$(echo $u_line | cut -d ' ' -f 5-)
echo "$i  $A" >> third_part
`rm -f tmp`
`touch tmp`
echo "Batch3" >> third_part
sed "1s/.*/2/" Batch3.txt > testfile.txt
`./nachos -F testfile.txt >> tmp`
u_line=$(tail -n 15 tmp | head -n 1)
A=$(echo $u_line | cut -d ' ' -f 5-)
echo "$i  $A" >> third_part
`rm -f tmp`
`touch tmp`
echo "Batch4" >> third_part
sed "1s/.*/2/" Batch4.txt > testfile.txt
`./nachos -F testfile.txt >> tmp`
u_line=$(tail -n 15 tmp | head -n 1)
A=$(echo $u_line | cut -d ' ' -f 5-)
echo "$i  $A" >> third_part
`rm -f tmp`
`touch tmp`
echo "Batch5" >> third_part
sed "1s/.*/2/" Batch5.txt > testfile.txt
`./nachos -F testfile.txt >> tmp`
u_line=$(tail -n 15 tmp | head -n 1)
A=$(echo $u_line | cut -d ' ' -f 5-)
echo "$i  $A" >> third_part
