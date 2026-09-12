
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#define A_FLAG 0x1000
#define B_FLAG 0x2000
#define X_FLAG 0x4000
#define Y_FLAG 0x8000
#define UP_FLAG 0x0001
#define DOWN_FLAG 0x0002
#define LEFT_FLAG 0x0004
#define RIGHT_FLAG 0x0008
#define LB_FLAG 0x0100
#define RB_FLAG 0x0200
#define PLAY_FLAG 0x0010
#define BACK_FLAG 0x0020
#define LS_CLK_FLAG 0x0040
#define RS_CLK_FLAG 0x0080
#define KEY_ACTION_DOWN 3
#define KEY_ACTION_UP 4
typedef struct input_data { short button; short lx, ly, rx, ry; char lt, rt; } input_data;
static char log_[4096]; static int n_;
void LiSendKeyboardEvent(short vk, char action, char mod){ n_+=sprintf(log_+n_, "%c%s ", (vk>=32&&vk<127)?vk:'#', action==KEY_ACTION_DOWN?"v":"^"); }
// Keyboard mode: translate the gamepad state into keyboard events.
// Needed for hosts that cannot inject virtual gamepads (Sunshine on macOS).
// Sticks are quantised to 4 keys each with a 50% threshold.
// ---------------------------------------------------------------------------
#define KBM_VK_BACK   0x08
#define KBM_VK_RETURN 0x0D
#define KBM_VK_SPACE  0x20
#define KBM_VK_LEFT   0x25
#define KBM_VK_UP     0x26
#define KBM_VK_RIGHT  0x27
#define KBM_VK_DOWN   0x28
#define KBM_STICK_THRESHOLD 16384   // of 32767 (read_analog output range)

typedef struct { uint32_t flag; short vk; } kbm_button_map;

static const kbm_button_map KBM_BUTTONS[] = {
  { A_FLAG,      KBM_VK_SPACE  }, // Cross    -> Space
  { B_FLAG,      'E'           }, // Circle   -> E
  { X_FLAG,      'Z'           }, // Square   -> Z
  { Y_FLAG,      'C'           }, // Triangle -> C
  { UP_FLAG,     KBM_VK_UP     },
  { DOWN_FLAG,   KBM_VK_DOWN   },
  { LEFT_FLAG,   KBM_VK_LEFT   },
  { RIGHT_FLAG,  KBM_VK_RIGHT  },
  { LB_FLAG,     'Q'           }, // L1 -> Q
  { RB_FLAG,     'R'           }, // R1 -> R
  { PLAY_FLAG,   KBM_VK_RETURN }, // Start  -> Enter
  { BACK_FLAG,   KBM_VK_BACK   }, // Select -> Backspace
  { LS_CLK_FLAG, 'F'           }, // L3 -> F
  { RS_CLK_FLAG, 'H'           }, // R3 -> H
};

// Bit layout of the virtual "key state" word built from the input_data.
enum {
  KBM_LT = 1u << 0, KBM_RT = 1u << 1,
  KBM_LX_NEG = 1u << 2, KBM_LX_POS = 1u << 3, KBM_LY_NEG = 1u << 4, KBM_LY_POS = 1u << 5,
  KBM_RX_NEG = 1u << 6, KBM_RX_POS = 1u << 7, KBM_RY_NEG = 1u << 8, KBM_RY_POS = 1u << 9,
};

static const kbm_button_map KBM_AXES[] = {
  { KBM_LT,     '1' }, // L2 -> 1
  { KBM_RT,     '3' }, // R2 -> 3
  { KBM_LX_NEG, 'A' }, { KBM_LX_POS, 'D' }, { KBM_LY_NEG, 'W' }, { KBM_LY_POS, 'S' }, // Vita Y: negative = up
  { KBM_RX_NEG, 'J' }, { KBM_RX_POS, 'L' }, { KBM_RY_NEG, 'I' }, { KBM_RY_POS, 'K' },
};

static uint32_t kbm_axis_state(const input_data *d) {
  uint32_t st = 0;
  if (d->lt != 0) st |= KBM_LT;   // lt/rt are 0 or 0xff (signed char → compare with 0)
  if (d->rt != 0) st |= KBM_RT;
  if (d->lx < -KBM_STICK_THRESHOLD) st |= KBM_LX_NEG; else if (d->lx > KBM_STICK_THRESHOLD) st |= KBM_LX_POS;
  if (d->ly < -KBM_STICK_THRESHOLD) st |= KBM_LY_NEG; else if (d->ly > KBM_STICK_THRESHOLD) st |= KBM_LY_POS;
  if (d->rx < -KBM_STICK_THRESHOLD) st |= KBM_RX_NEG; else if (d->rx > KBM_STICK_THRESHOLD) st |= KBM_RX_POS;
  if (d->ry < -KBM_STICK_THRESHOLD) st |= KBM_RY_NEG; else if (d->ry > KBM_STICK_THRESHOLD) st |= KBM_RY_POS;
  return st;
}

static void kbm_send_diff(uint32_t now, uint32_t before, const kbm_button_map *tbl, size_t n) {
  for (size_t i = 0; i < n; i++) {
    uint32_t was = before & tbl[i].flag, is = now & tbl[i].flag;
    if (is && !was)      LiSendKeyboardEvent(tbl[i].vk, KEY_ACTION_DOWN, 0);
    else if (!is && was) LiSendKeyboardEvent(tbl[i].vk, KEY_ACTION_UP, 0);
  }
}

// Emit key transitions between `before` and `now`; returns nothing, purely side effects.
static void keyboard_mode_send(const input_data *now, const input_data *before) {
  kbm_send_diff(now->button, before->button, KBM_BUTTONS, sizeof(KBM_BUTTONS)/sizeof(KBM_BUTTONS[0]));
  kbm_send_diff(kbm_axis_state(now), kbm_axis_state(before), KBM_AXES, sizeof(KBM_AXES)/sizeof(KBM_AXES[0]));
}

// Release every key we might be holding (used when leaving the stream / opening overlays).
static void keyboard_mode_release_all(void) {
  input_data none; memset(&none, 0, sizeof(none));
  input_data all;  memset(&all, 0, sizeof(all));
  all.button = 0xFFFF; all.lt = all.rt = 0xFF;
  // Axis "all pressed" cannot be represented (neg and pos are exclusive) → do two passes.
  all.lx = all.ly = all.rx = all.ry = 32767;  keyboard_mode_send(&none, &all);
  all.lx = all.ly = all.rx = all.ry = -32767; keyboard_mode_send(&none, &all);
}


static int fails=0;
#define EXPECT(s) do{ if(strcmp(log_,s)){printf("FAIL: got '%s' want '%s'\n",log_,s);fails++;} else printf("ok: %s\n",s); n_=0; log_[0]=0; }while(0)
int main(){
  input_data a={0},b={0};
  // 1) cross press → Space down; release → Space up
  b.button=A_FLAG; keyboard_mode_send(&b,&a); EXPECT(" v ");
  keyboard_mode_send(&a,&b); EXPECT(" ^ ");
  // 2) left stick up past threshold → W down; below threshold → W up
  memset(&a,0,sizeof a); memset(&b,0,sizeof b);
  b.ly=-20000; keyboard_mode_send(&b,&a); EXPECT("Wv ");
  a=b; b.ly=-5000; keyboard_mode_send(&b,&a); EXPECT("W^ ");
  // 3) stick swings from left to right in one frame → A up + D down
  memset(&a,0,sizeof a); memset(&b,0,sizeof b);
  a.lx=-30000; b.lx=30000; keyboard_mode_send(&b,&a); EXPECT("A^ Dv ");
  // 4) trigger 0xff (signed char -1) is detected as pressed
  memset(&a,0,sizeof a); memset(&b,0,sizeof b);
  b.lt=(char)0xff; keyboard_mode_send(&b,&a); EXPECT("1v ");
  // 5) no change → no events
  keyboard_mode_send(&b,&b); EXPECT("");
  // 6) release_all after holding cross + stick up + trigger → all three get key-up
  memset(&a,0,sizeof a); memset(&b,0,sizeof b);
  b.button=A_FLAG; b.ly=-30000; b.rt=(char)0xff; keyboard_mode_send(&b,&a); n_=0; log_[0]=0;
  keyboard_mode_release_all();
  int ok = strstr(log_," ^ ")&&strstr(log_,"W^ ")&&strstr(log_,"3^ ")&&!strstr(log_,"v ");
  printf("%s: release_all → '%s'\n", ok?"ok":"FAIL", log_); if(!ok)fails++;
  printf("\n%d failure(s)\n", fails); return fails;
}
