trap "" INT
xrandr --output DP-1-0 --mode 1920x1080 --rate 165 --above eDP-1 &
setxkbmap -layout us,ru -option grp:alt_shift_toggle
xwallpaper --zoom ~/wallpaper.jpg &
(sleep 3 && picom --vsync --backend glx) &
dwmbar &
/usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1 &
~/.local/bin/lock-startup.sh &
exec dwm
