#ifndef BADAPPLE_TERM_CONTROL
#define BADAPPLE_TERM_CONTROL

// clear entire screen
void clearn_entire_screen()
{
  printf("\x1b[2J");
}

// restore terminal
void restore_term() {
  printf("\033[?25h\033[0m\033[H\033[2J");
  printf("\x1b[?47l");
}

#endif /* ifndef BADAPPLE_TERM_CONTROL */
