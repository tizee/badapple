// for parsing cli parameters
#include <getopt.h>
// terminal info
#include <termios.h>
// handling terminal event via signal
#include <signal.h>
// useful utilities
#include <stdlib.h>
// time
#include <time.h>

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <ctype.h>

#include "control.c"
#include "data.c"

// assume the size of terminal is the same as the size of frame
int term_h = FRAME_HEIGHT;
int term_w = FRAME_WIDTH;
int min_col = -1;
int max_col = -1;
int min_row = -1;
int max_row = -1;

const char *white_bg = "\x1b[48;5;15m ";
const char *black_fg = "\x1b[48;5;15m\x1b[38;5;16m";
const char *reset_color = "\x1b[0m";


int clear_screen = 1;
int playing = 1;

typedef struct {
    time_t now;
   time_t last;
   int count;
} Timer;

double fps= (double)1/(double)30;
Timer fps_timer;

void init_fps_timer(){
  fps_timer.count=1;
  time(&fps_timer.now);
  fps_timer.last=fps_timer.now;
}

void stop_with_fps_timer() {
  time_t current;
  // seconds since epoch
  time(&current);
  long stop = fps_timer.count * fps + fps_timer.last - current;
  if (stop <0){
    time(&current);
    fps_timer.now= current;
    fps_timer.last=fps_timer.now;
    fps_timer.count=0;
    return;
  }
  usleep((long)stop*2000);
  time(&current);
  fps_timer.now= current;
  fps_timer.count+=1;
}

// handle int signal
void SIGINT_handler(int sig) {
  restore_term();
  exit(0);
}

void get_terminal_size() {
  struct winsize ws;
  ioctl(0, TIOCGWINSZ, &ws);
  term_h = ws.ws_row;
  term_w = ws.ws_col;
}

// handle terminal window size change
void SIGWINCH_handler(int sig) {
  get_terminal_size();
  min_col = (term_w - FRAME_WIDTH) / 2;
  max_col = (term_w + FRAME_WIDTH) / 2;

  min_row = (term_h - FRAME_HEIGHT) / 2;
  max_row = (term_h + FRAME_HEIGHT) / 2;

  signal(SIGWINCH, SIGWINCH_handler);
}

int digits(int val) {
  int d = 1, c;
  if (val >= 0)
    for (c = 10; c <= val; c *= 10)
      d++;
  else
    for (c = -10; c >= val; c *= 10)
      d++;
  return (c < 0) ? ++d : d;
}

time_t current;
time_t start;
void ba_time_counter() {
  time(&current);
  double diff = difftime(current, start);
  int len = digits(diff);
  int width = (term_w - len) / 2;
  while (width > 0) {
    printf(" ");
    width--;
  }
  printf("\033[1;30mRunning for %0.0f seconds\033[J\033[0m", diff);
}

// play frames
void play() {
  init_fps_timer();
  printf("\x1b[?25l");
  printf("\x1b[?47h");

  int current_frame = 0;
  while (playing) {
    if (clear_screen) {
      printf("\033[H");
    } else {
      printf("\033[u");
    }
    for (int y = min_row; y < max_row; ++y) {
      if (y < 0) {
        continue;
      }
      if (y >= term_h - 1) {
        break;
      }
      for (int x = min_col; x < max_col; ++x) {
        if (x < 0) {
          continue;
        }
        if (x >= term_w - 1) {
          break;
        }
        char ch = frames[current_frame][y-min_row][x-min_col];
        if (ch != ' ') {
          printf("%s%c",black_fg,ch);
        } else {
          printf("%s",white_bg);
        }
      }
      putc('\n', stdout);
    }
    ba_time_counter();
    current_frame+=1;
    if (!frames[current_frame]) {
      current_frame = 0;
    }
    stop_with_fps_timer();
  }
}


int main(int argc, char *argv[]) {
  char *term = NULL;
  term = getenv("TERM");
  time(&start);

  get_terminal_size();
  printf("%d %d\n", term_h, term_w);
  min_col = (term_w - FRAME_WIDTH) / 2;
  max_col = (term_w + FRAME_WIDTH) / 2;

  min_row = (term_h - FRAME_HEIGHT) / 2;
  max_row = (term_h + FRAME_HEIGHT) / 2;
  /* Accept ^C -> restore cursor */
  signal(SIGINT, SIGINT_handler);
  play();
  return 0;
}
