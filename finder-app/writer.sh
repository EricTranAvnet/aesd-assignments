#!/bin/bash

writefile="$1"
writestr="$2"

# I use google to find dirname function
writepath="$(dirname "${writefile}")"
filename="$(basename "${writefile}")"

if [ -z "$writefile" ] || [ -z "$writestr" ] || [ -d "$writefile" ]
then
	echo "Not enough parameters / wrong parameters"
	exit 1
fi

mkdir -p "$writepath"
echo "Writing "$writestr" > "$writepath"/"${filename}" " 
echo "$writestr" > "$writepath"/"${filename}"



