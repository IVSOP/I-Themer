#!/bin/bash

rofi -show theme-menu -modi theme-menu:$HOME/I-Themer/themer -theme $HOME/.config/i3/themes/current-theme/dmenu-theme.rasi -font "cantarell regular 15" -show-icons -i
if [ $# -eq 1 ]
then
	i3-msg restart &>/tmp/rofi-theme.txt
fi
