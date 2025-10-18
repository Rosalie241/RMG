#!/usr/bin/env bash
file="./mupen64plus.cfg"

device_name=""
current_button=""

echo "    {"

while read -r line
do
	if [[ "$line" == "DeviceName ="* ]] ||
		[[ "$line" == "DeviceType ="* ]]
	then
		key="$(cut -d'=' -f1 <<< $line | tr -d ' ')"
		value="$(cut -d'=' -f2 <<< $line | awk '{$1=$1};1')"
		if [[ "$line" == "DeviceName ="* ]]
		then
			value="[ $value ]"
		fi
		echo "        \"$key\": $value,"
	elif [[ "$line" == "A_"* ]] ||
			[[ "$line" == "B_"* ]] ||
			[[ "$line" == "Start_"* ]] || 
			[[ "$line" == "Dpad"* ]] ||
			[[ "$line" == "CButton"* ]] ||
			[[ "$line" == *"Trigger"* ]] ||
			[[ "$line" == "AnalogStick"* ]]
 	then
		key="$(cut -d'=' -f1 <<< $line | tr -d ' ')"
		value="$(cut -d'=' -f2 <<< $line | tr -d ' ')"

		echo "        \"$key\": $value,"
	fi
done < "$file"

echo "    },"

