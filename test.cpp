#include <iostream>
#include <cmath>
#include <locale>
#include <unistd.h>
#include "ctext.h"

int8_t my_event(ctext *context, ctext_event event)
{
  /*
  switch(event)
  {
    case CTEXT_SCROLL:
      printf("< Scroll >");
      break;

    case CTEXT_CLEAR:
      printf("< Clear >");
      break;

    case CTEXT_DATA:
      printf("< Data >");
      break;
  }
  */
  return 0;
}

int main(int argc, char **argv ){
  locale::global(locale("en_US.utf8"));

  initscr();  
  cbreak();      
  curs_set(0);
  WINDOW *local_win;
  ctext_config config;

  local_win = newwin(7, 9, 5, 5);
  //box(local_win, 0 , 0);
  wrefresh(local_win);
  start_color();
  init_pair(1,COLOR_WHITE, COLOR_BLUE);

  ctext ct(local_win);

  // get the default config
  ct.get_config(&config);

  // add my handler
  config.m_on_event = my_event;
  config.m_bounding_box = false;
  config.m_scroll_on_append = true;
  //config.m_do_wrap = true;
  config.m_append_top = true;
  
  // set the config back
  ct.set_config(&config);

  int speed = 150000;
  int8_t x = 0;
    wbkgd(local_win,COLOR_PAIR(1));
  for(x = 0; x < 15; x++) {
    ct.printf("hello %d world", x);
    usleep(speed);
  }

  for(x = 0; x < 5; x++) {
    usleep(speed);
    ct.right();
    usleep(speed);
    ct.down();
  }
  for(x = 0; x < 15; x++) {
    usleep(speed);
    ct.left();
    usleep(speed);
    ct.up();
  }

  for(x = 0; x < 18; x++) {
    ct.clear(1);
    usleep(speed);
  }

  endwin();
  return 0;
}
