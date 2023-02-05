for i in $(cat ./Scripts/test_info.txt); do env ROFI_INFO="$i" ./Scripts/valgrind.sh; done
