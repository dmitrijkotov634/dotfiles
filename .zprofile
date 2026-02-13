source /opt/v2rayn_env.sh
export PATH="/opt/scripts:$PATH"
export PATH="$HOME/.local/bin:$PATH"
[[ -z $DISPLAY && $XDG_VTNR -eq 1 ]] && startx
