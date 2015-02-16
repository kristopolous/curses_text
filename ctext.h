#include <vector>
#include <ncursesw/ncurses.h>

#ifndef __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT
#define __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT

using namespace std;

class Ctext {
  public:
    Ctext();

    int clear();
    // scrolling
    int scroll_to(int x, int y);
    int get_offset(int*x, int*y); 
    int get_size(int*x, int*y);

    int up(int amount = 1);
    int down(int amount = 1);
    int left(int amount = 1);
    int right(int amount = 1);

    int printf(const char*format, ...);


//  private:
    int render();

    int m_tabstop;
    bool m_do_wrap;
    int m_scrollback;
    bool m_append_top;
    bool m_scroll_on_append;

    int m_offset_x;
    int m_offset_y;

    int m_size_x;
    int m_size_y;

    vector<wstring> m_buffer;
};

int cprintf(Ctext*win, const char *format, ...);

#endif
