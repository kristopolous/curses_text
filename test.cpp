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

  local_win = newwin(7, 8, 5, 5);
  //box(local_win, 0 , 0);
  wborder(local_win, '|', '|', '-', '-', '+', '+', '+', '+');
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
//  config.m_do_wrap = true;
  config.m_append_top = true;
  
  // set the config back
  ct.set_config(&config);

  int8_t x = 0;
    wbkgd(local_win,COLOR_PAIR(1));
  for(x = 0; x < 15; x++) {
    ct.printf("hello %d world", x);
    usleep(200000);
  }

  for(x = 0; x < 15; x++) {
    usleep(200000);
    ct.right();
    usleep(100000);
    ct.left();
   // ct.down();
  }

  return 0;
}
