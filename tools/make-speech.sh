#!/bin/bash

voice="$1"
conf_file="phrases"
ext="ogg"
conf_file_voice="$voice/$conf_file"
speak_cmd="say"
speak_cmd_fopt="-o"

echo "Usage:"
echo "$0 [-u] <voice>"
echo "<voice> is a voice/folder to be created or updated."
echo "Use option -u to update."
echo "Edit file '$conf_file' to add/remove strings."

#----------------------------------------------
# read conf_file file and say lines...
echo "Press <Enter> to say again,"
echo " or type another text to speak,"
echo " or type 'y' to confirm and save phrase,"
echo " or type 'c' to update default substitutions too."


while getopts 'u:' flag; do
  case "${flag}" in
    u) do_update=1; voice="$OPTARG"; conf_file_voice="$voice/speech" ;;
    *) echo "Unexpected option ${flag}"; exit ;;
  esac
done


input=""	# user input string
IFS=","
while read -r console_text say_text
do
  [[ ${console_text:0:1} == '#' ]] && continue
  [ -z "$console_text" ] && continue
  [ -z $say_text ] && say_text="$console_text"
  #--------------------------
	#check voice config file for substitutes
	if [ -f $conf_file_voice ]
	then
		while read -r vconf
		do
			[[ ${vconf:0:1} == '#' ]] && continue
			[ -z "$vconf" ] && continue
			[[ "$vconf" == *,* ]] || continue
			vct=$(echo "$vconf" | cut -f1 -d,)
			vst=$(echo "$vconf" | cut -f2 -d,)
			[ -z "$vst" ] && continue
			[ "$console_text" == "$vct" ] || continue
			say_text="$vst"
			break
		done < $conf_file_voice
	fi

  #--------------------------
  text="$say_text"
	while true; do
	  [ "$console_text" == "$text" ] && echo -n "$console_text: " || echo -n "$console_text ($text): "
		if [ "$do_update" ]
		then
		  echo "[update]"
		  input="y"
		else
		  $speak_cmd "$text"
		  read input </dev/tty
  		[ "$input" == "" ] && continue
		fi
		if [[ "$input" == "c" ]]
		then
			[ "$text" == "$console_text" ] && newline="${console_text}" || newline="${console_text},${text}"
			sed -e "s/.*$console_text.*/${newline}/" -i '' $conf_file
			input="y"
		fi
		if [[ "$input" == "y" ]]
		then
			mkdir -p "$voice"
			fname="$voice/$console_text.$ext"
			$speak_cmd $speak_cmd_fopt "$fname" "$text"
			#update conf_file with new string
			if [ "$say_text" != "$text" ];  # update needed
			then
			  [ -f $conf_file_voice ] && sed -e "/${console_text}/d" -i '' $conf_file_voice
			  [ "$console_text" == "$text" ] || echo "${console_text},${text}" >> $conf_file_voice
			fi
			break
		fi
		text="$input"
  done
#--------------------------
done < $conf_file
