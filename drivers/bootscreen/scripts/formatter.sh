#!/bin/bash


if [ -z $1 ]; then
	echo "no file to read" 1>&2
	exit 1
fi

#Constants
LINES_PER_SCREEN_WIDTH=8
MAX_NUM=8

#Variables
DATA=`grep -o '\b0x\w*' $1`
LINE_PATTERN="/* Line: %u */\n"
COUNTER=0
WIDTH_LINE_CNT=0
LINE_CNT=1

printf "$LINE_PATTERN" $LINE_CNT

for d in $DATA
do
	((COUNTER++))
	printf "%s," $d

	if (( $COUNTER == $MAX_NUM )); then
		echo
		COUNTER=0
		((WIDTH_LINE_CNT++))

		if (( $WIDTH_LINE_CNT == $LINES_PER_SCREEN_WIDTH )); then
			WIDTH_LINE_CNT=0
			((LINE_CNT++))
			printf "$LINE_PATTERN" $LINE_CNT
		fi
	else
		printf " "
	fi
done
