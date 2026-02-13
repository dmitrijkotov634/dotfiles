setopt PROMPT_SUBST

autoload -Uz vcs_info
precmd() { vcs_info }
zstyle ':vcs_info:git:*' formats ' %b'

PROMPT='%~${vcs_info_msg_0_} > '

source /opt/v2rayn_env.sh
export PATH="/opt/scripts:$PATH"
export PATH="$HOME/.local/bin:$PATH"
