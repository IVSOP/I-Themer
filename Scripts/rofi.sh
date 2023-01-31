#!/bin/bash

rofi -show theme-menu -modi theme-menu:$HOME/I-Themer/themer -theme\
	$HOME/.config/rofi/current-theme/dmenu-theme.rasi -font "cantarell regular 15"\
	-show-icons -i #&& i3-msg restart &>/tmp/rofi-theme.txt
