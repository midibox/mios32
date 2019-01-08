echo "-------------------------------------------------------------------------------"
echo "Converting call graphs to images"
echo "-------------------------------------------------------------------------------"
echo "Enter file format(s) would you like to create, separated by <space>"
echo "eg: pdf png hpgl"
echo "Then press <enter>"

set -a dotoutput
read -p"Filetypes:  " -a dotoutput
element_count=${#dotoutput[@]}
index=0
while [ "$index" -lt "$element_count" ]
do
echo "Converting call graphs to "${dotoutput[$index]}" format"
    dot egypt/*.dot -O -T${dotoutput[$index]}
    let "index += 1"
done
unset dotoutput
