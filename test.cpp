#include <locale>
#include <string.h>
#include <unistd.h>
#include <assert.h>
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
  int speed = 450000;
  int8_t x = 0;
  int amount = 0;
  locale::global(locale("en_US.utf8"));

  initscr();  
  cbreak();      
  curs_set(0);
  WINDOW *local_win;
  ctext_config config;

  local_win = newwin(9, 100, 5, 5);
  start_color();

  ctext ct(local_win);

  // get the default config
  ct.get_config(&config);

  // add my handler
  config.m_on_event = my_event;
  //config.m_bounding_box = true;
  config.m_buffer_size = 100;
  config.m_scroll_on_append = true;
  //config.m_do_wrap = true;
  config.m_append_top = true;
  
  // set the config back
  ct.set_config(&config);

  attr_t attrs; short color_pair_number;
  for(x = 0; x < 100; x++) 
  {
    init_pair(x, x, 0);
    /*
    wattr_on(local_win, COLOR_PAIR(x), 0);
    waddch(local_win, 'a');
    wattr_get(local_win, &attrs, &color_pair_number, 0);
    printf("%x %x", attrs, color_pair_number);
    wattr_off(local_win, COLOR_PAIR(x), 0);
    wrefresh(local_win);
    usleep(speed / 200);
    */
  }


  char buffer[32], *ptr;
  for(x = 0; x < 15; x++) {
  //  wattr_on(local_win, COLOR_PAIR(x), 0);
    //memset(buffer, 0, 32);
   ct.printf("%x%x%x%c%c%c\n", 
        x, x, x, 
        x + 'D', x + 'D', x + 'D');

   // wattr_off(local_win, COLOR_PAIR(x), 0);
    //usleep(speed / 15);
    /*
    for(ptr = buffer; *ptr; ptr++) {
      ct.putchar((char)*ptr);
      usleep(speed / 15);
    }
    */
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
    amount = ct.clear(1);
    /*
    if(x < 15) 
    {
      assert(amount == 1);
    }
    else
    {
      assert(amount == 0);
    }
    */
    usleep(speed);
  }

  endwin();
  return 0;
}
