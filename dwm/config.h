#include <X11/XF86keysym.h>

/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[] = { "FiraCode Nerd Font:size=11" };
static const char dmenufont[]       = "FiraCode Nerd Font:size=11";

static const unsigned int gappx     = 8;

/* palette matched to wallpaper */
static const char col_bg[]          = "#0e1225";  /* deep navy, dark base */
static const char col_bg_alt[]      = "#1c2340";  /* slightly lighter border */
static const char col_fg[]          = "#9ea5b8";  /* muted text, blends with wallpaper */
static const char col_fg_bright[]   = "#f2eae7";  /* secondary wallpaper color — bright text */
static const char col_accent[]      = "#5570c9";  /* primary wallpaper color — active accent */

static const char *colors[][3]      = {
	/*               fg              bg           border     */
	[SchemeNorm] = { col_fg,         col_bg,      col_bg_alt },
	[SchemeSel]  = { col_fg_bright,  col_accent,  col_accent },
};

/* tagging — 5 workspaces with emoji */
static const char *tags[] = { "\uf0ac", "\uf120", "\uf07b", "\uf075", "\uf11b", "\uf044", "\uf013" };

static const Rule rules[] = {
	/* class      instance    title       tags mask     isfloating   monitor */
	{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",  NULL,       NULL,       1 << 0,       0,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55;
static const int nmaster     = 1;
static const int resizehints = 1;
static const int lockfullscreen = 1;

static const Layout layouts[] = {
	{ "[]=",      tile },
	{ "><>",      NULL },
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0";
static const char *dmenucmd[]    = { "rofi", "-show", "run", NULL };
static const char *termcmd[]     = { "alacritty", NULL };
static const char *filecmd[] = { "pcmanfm", NULL };
static const char *vol_up[]   = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL };
static const char *vol_down[] = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL };
static const char *vol_mute[] = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };
static const char *powertoggle[] = { "/home/dmitry/.local/bin/power-toggle", NULL };
static const char *lockcmd[] = { "/home/dmitry/.local/bin/lock.sh", NULL };

/* screenshot: area selection → clipboard + file */
static const char *screenshot[]  = { "sh", "-c",
	"maim -s | tee ~/screenshots/$(date +%Y%m%d_%H%M%S).png | xclip -selection clipboard -t image/png",
	NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ 0,                            XK_Print,  spawn,          {.v = screenshot } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
        { 0, XF86XK_MonBrightnessUp,   spawn, SHCMD("brightnessctl set +10%") },
        { 0, XF86XK_MonBrightnessDown, spawn, SHCMD("brightnessctl set 10%-") },
        { 0, XF86XK_AudioRaiseVolume,  spawn, {.v = vol_up} },
        { 0, XF86XK_AudioLowerVolume,  spawn, {.v = vol_down} },
        { 0, XF86XK_AudioMute,         spawn, {.v = vol_mute} },
        { MODKEY, XK_F5, spawn, {.v = powertoggle } },
        { MODKEY|ShiftMask, XK_l, spawn, {.v = lockcmd } },
        { MODKEY,                       XK_e,      spawn,          {.v = filecmd } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
        TAGKEYS(                        XK_7,                      6)
        { MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
static const Button buttons[] = {
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
