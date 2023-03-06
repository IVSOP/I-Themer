#!/bin/bash

# insert your rofi theme instead of "$HOME/.config/rofi/current-theme/dmenu-theme.rasi"
# change "$HOME/I-Themer/ithemer" if needed
rofi -show theme-menu -modi theme-menu:$HOME/I-Themer/ithemer -theme\
	$HOME/.config/rofi/current-theme/dmenu-theme.rasi -font "cantarell regular 15"\
	-show-icons -i #&& i3-msg restart &>/tmp/rofi-theme.txt
