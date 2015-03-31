#include <locale>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#define CURSES_CTEXT_DEBUG 1
#include "ctext.h"

#define page 4096
int from_stdin(ctext*ct)
{
	char buffer[page];
	FILE *f = fopen("ulysses.txt", "r");
	while(fgets(buffer, page, f)) {
		ct->printf("%s", buffer);
	}
	fclose(f);
}
		
int main(int argc, char **argv ){
  FILE *pDebug;
  int speed = 450000;
	int8_t ret;
  int16_t x = 0;
  int16_t y = 0;
  int amount = 0;
  float perc;
  locale::global(locale("en_US.utf8"));

  initscr();  
  cbreak();      
  curs_set(0);
  WINDOW *local_win;
  ctext_config config;
  pDebug = fopen("debug1.txt", "a");

  local_win = newwin(9, 90, 5, 5);
  start_color();

  ctext ct(local_win);

  // get the default config
  ct.get_config(&config);

  // add my handler
  config.m_bounding_box = true;
  config.m_buffer_size = 300;
  //config.m_scroll_on_append = true;
  config.m_do_wrap = true;
	config.m_auto_newline = false;
  //config.m_append_top = true;
  
  // set the config back
  ct.set_config(&config);

  attr_t attrs; short color_pair_number;
  for(x = 0; x < 200; x++) 
  {
    init_pair(x, x, 0);

    /*
    wattr_on(local_win, COLOR_PAIR(x), 0);
    wattr_get(local_win, &attrs, &color_pair_number, 0);
    //printf("%x %x ", attrs,color_pair_number);
    wattr_off(local_win, COLOR_PAIR(x), 0);
i
    //wattr_on(local_win, COLOR_PAIR(x), 0);
    wattr_on(local_win, COLOR_PAIR(color_pair_number), 0);
    wattr_get(local_win, &attrs, &color_pair_number, 0);
    printf("%x %x\r\n", attrs,color_pair_number);
    wattr_off(local_win, COLOR_PAIR(x), 0);
    mvwaddwstr(local_win, 0, x, L"h");
    wattr_off(local_win, COLOR_PAIR(x), 0);
    wrefresh(local_win);
    usleep(speed / 200);
*/
  }

	int32_t loop = 0;
  char buffer[32], *ptr;
	char testLen[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  int color = 0, round;
  ct.ob_start();
if(0) {
  for(y = 0; y < 250; y++) {
    //fprintf(pDebug, "%d\n", ct.available_rows());
    x = y % 50;
    //ct.printf("%4d", y);
		/*
    for(round = 0; round < 2; round++) 
    {
      wattr_on(local_win, COLOR_PAIR(color % 200), 0);
      ct.printf("%c", (color % 26) + 'A');//, testLen + ((color*4) %26) );
      wstandend(local_win);
      //wattr_off(local_win, COLOR_PAIR(color % 200), 0);
      color++;
    }
		*/

		fprintf(pDebug, "%04x ", COLOR_PAIR(x * 3 + 1));
		fflush(pDebug);
    wattr_on(local_win, COLOR_PAIR(x * 3 + 1), 0);

		for(loop = 1; loop < 9; loop++) {
			ct.printf("%d%c%d%c::", loop, loop + 'A', loop, loop + 'a');
		}
		ct.printf("\n");
    wstandend(local_win);
    /*
    for(ptr = buffer; *ptr; ptr++) {
      ct.putchar((char)*ptr);
      usleep(speed / 15);
    }
    */
  }
}
	from_stdin(&ct);

  ct.ob_end();

	ctext_search searcher;
	ct.new_search(&searcher, "the");

	x = 0;
	do {
		ret = ct.str_search(&searcher);
		usleep(speed * 5);

		/*
  	for(x = 0; x < 10; x++) {
			ct.down();
			usleep(speed * 5);
			x++;
			if(x == 5)
			{
				ct.search_off();
			}
		}
		*/

	} while (!ret);

  endwin();
		return(0);

  for(x = 0; x < 100; x++) {
    //ct.right();
    ct.down();
    usleep(speed / 5);
    //ct.page_down();
    //usleep(speed * 1);
  }

	/*
  for(x = 0; x < 20; x++) {
    ct.get_offset_percent(&perc);
    fprintf(pDebug, "%f\n", perc);
    fflush(pDebug);

    ct.left();
    ct.page_down();
    usleep(speed);
  }
  fclose(pDebug);

  for(x = 0; x < 18; x++) {
    amount = ct.clear(1);
    if(x < 15) 
    {
      assert(amount == 1);
    }
    else
    {
      assert(amount == 0);
    }
    usleep(speed);
  }
    */

  endwin();
  return 0;
}
