#include <iostream>
#include <cmath>
#include <locale>
#include <unistd.h>
#include "ctext.h"

ctext_cb my_event(ctext *context, ctext_event event)
{
  switch(event)
  {
    case ctext_event::SCROLL:
      printf("< Scroll >");
      break;

    case ctext_event::CLEAR:
      printf("< Clear >");
      break;

    case ctext_event::DATA:
      printf("< Data >");
      break;
  }
  return 0;
}

int main(int argc, char **argv ){
  locale::global(locale("en_US.utf8"));

  initscr();  
  cbreak();      
  curs_set(0);
  WINDOW *local_win;
  ctext_config config;

  local_win = newwin(10, 60, 5, 5);
  box(local_win, 0 , 0);

  ctext ct(local_win);

  // get the default config
  ct.get_config(&config);

  // add my handler
  config.m_on_event = my_event;
  
  // set the config back
  ct.set_config(&config);

  ct.printf("hello world");

  return 0;
}
