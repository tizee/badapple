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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <ctype.h>
#include <sys/ioctl.h>

#include "control.c"
#include "data.c"

// assume the size of terminal is the same as the size of frame
int term_h = FRAME_HEIGHT;
int term_w = FRAME_WIDTH;
int min_col = -1;
int max_col = -1;
int min_row = -1;
int max_row = -1;

int start_frame = 0;
int end_frame = 6571;

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

double fps = (double)1 / (double)30;
Timer fps_timer;

void init_fps_timer() {
  fps_timer.count = 1;
  time(&fps_timer.now);
  fps_timer.last = fps_timer.now;
}

void stop_with_fps_timer() {
  time_t current;
  // seconds since epoch
  time(&current);
  long stop = fps_timer.count * fps + fps_timer.last - current;
  if (stop < 0) {
    time(&current);
    fps_timer.now = current;
    fps_timer.last = fps_timer.now;
    fps_timer.count = 0;
    return;
  }
  usleep((long)stop * 2000);
  time(&current);
  fps_timer.now = current;
  fps_timer.count += 1;
}

// handle int signal
void SIGINT_handler(int sig) {
  restore_term();
  exit(0);
}

void fill_white_bg(){
  for(int y = 0 ; y<term_h;y++){
    for(int x=0;x<term_w;x++){
      printf("%s",white_bg);
    }
  }
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
  clearn_entire_screen();
  fill_white_bg();

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
  int padding = term_w / 2 - len / 2 - 12;
  printf("\x1b[%d;0H", term_h - 1);
  while (padding > 0) {
    printf(" ");
    padding--;
  }
  printf("\x1b[1;30mRunning for %0.0f seconds\x1b[J\x1b[0m", diff);
}

// play frames
void play() {
  init_fps_timer();
  // save cursor and termianl info
  printf("\x1b[?25l");
  printf("\x1b[?47h");

  int current_frame = start_frame;
  fill_white_bg();
  while (playing) {
    if (clear_screen) {
      printf("\x1b[H");
    } else {
      printf("\x1b[u");
    }
    for (int y = min_row; y < max_row; y++) {
      for (int x = min_col; x < max_col; x++) {
          printf("\x1b[%d;%dH", y, x);
          int xx = x - min_col < FRAME_WIDTH ? x - min_col : FRAME_WIDTH - 1;
          int yy = y - min_row < FRAME_HEIGHT ? x - min_row : FRAME_HEIGHT - 1;
          char ch = frames[current_frame][y - min_row][x - min_col];
          if (ch != ' ') {
            printf("%s%c", black_fg, ch);
          } else {
            printf("%s", white_bg);
          }
        }
      putc('\n', stdout);
    }
    ba_time_counter();
    current_frame += 1;
    if (!frames[current_frame] || current_frame >= end_frame) {
      current_frame = start_frame;
    }
    stop_with_fps_timer();
  }
}

int main(int argc, char *argv[]) {
  char *term = NULL;
  term = getenv("TERM");
  time(&start);

  static struct option long_opts[] = {
      {"start-frame", required_argument, 0, 's'},
      {"end-frame", required_argument, 0, 'e'},
  };

  int index, c;
  while ((c = getopt_long(argc, argv, "s:e:", long_opts, &index)) != -1) {
    if (!c) {
      if (long_opts[index].flag == 0) {
        c = long_opts[index].val;
      }
    }
    switch (c) {
    case 'e':
      end_frame = atoi(optarg);
      break;
    case 's':
      start_frame = atoi(optarg);
      break;
    default:
      break;
    }
  }

  get_terminal_size();
  min_col = (term_w - FRAME_WIDTH) / 2;
  max_col = (term_w + FRAME_WIDTH) / 2;

  min_row = (term_h - FRAME_HEIGHT) / 2;
  max_row = (term_h + FRAME_HEIGHT) / 2;
  signal(SIGINT, SIGINT_handler);
  signal(SIGWINCH, SIGWINCH_handler);
  play();
  return 0;
}
