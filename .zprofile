[[ -f ~/.zshrc ]] && source ~/.zshrc
[[ -z $DISPLAY && $XDG_VTNR -eq 1 ]] && startx
