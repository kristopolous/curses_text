#include <vector>
#include <stdint.h>
#include <ncursesw/ncurses.h>

#ifndef __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT
#define __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT

using namespace std;

typedef vector<wstring> ctext_buffer;
typedef int8_t* ctext_cb;

enum ctext_event {
  SCROLL,
  CLEAR,
  DATA
};

struct ctext_config_struct
{
  // 
  // If the content has a tab, '\t' contained
  // within it, what should the margin be.
  //
  int8_t m_tabstop;
#define CTEXT_DEFAULT_TABSTOP  2

  //
  // This specifies how many lines are kept
  // in the ring-buffer.  
  //
  size_t m_scrollback;
#define CTEXT_DEFAULT_SCROLLBACK 100

  //
  // The bounding box bool specifies whether
  // we are allowed to move outside where the
  // content exists.  Pretend there's the 
  // following content and window (signified
  // by the + marks)
  //
  //  +     +
  //   xxxx
  //   xxx
  //   xxxxx
  //  +     +
  //
  // If we were to move say, 3 units right and
  // had no bounding box (false) you'd see this:
  //
  //  +     +
  //   x
  //
  //   xx
  //  +     +
  //
  // In other words, we could potentially scroll
  // well beyond our content.
  //
  // A bounding box set to true would prevent this,
  // making sure that the viewport doesn't extend
  // beyond the existing content.
  //
  bool m_bounding_box;
#define CTEXT_DEFAULT_BOUNDING_BOX false

  // 
  // Sometimes content can be extremely lengthy 
  // on one line, overwhelming any other content
  // in view, losing the context of what the content
  // means.
  //
  // In these cases, we can truncate long text from
  // occupying the next row of text, and instead extend
  // beyond the viewport of our window.  Under these cases
  // the user will have to scroll the viewport in order
  // to see the remainder of the text.
  //
  bool m_do_wrap;
#define CTEXT_DEFAULT_DO_WRAP false

  //
  // In most user interfaces, new text appears 
  // underneath previous text on a new line.
  //
  // However, sometimes it's more natural to see
  // new entries come in ON TOP of old ones, pushing
  // the old ones downward.
  //
  // m_append_top deals with this duality
  //
  bool m_append_top;
#define CTEXT_DEFAULT_APPEND_TOP false

  //
  // Sometimes seeing new content is of the utmost
  // importance and takes precendence over analysis
  // of any historical data.
  //
  // In that case, the scroll_on_append will forcefully
  // scroll the text so that the new content is 
  // visible.
  //
  bool m_scroll_on_append;
#define CTEXT_DEFAULT_APPEND_TOP false

  //
  // The following function pointer, if defined
  // is executed when an event happens as defined
  // in the ctext_event enum above.
  //
  // If the pointer is not set, then no callback
  // is called.
  //
  // No meta-data travels with the event other than
  // the callers context and the nature of the event.
  //
  // The context can be queried based on the event
  //
  ctext_cb (*m_on_event)(ctext *context, ctext_event event);
#define CTEXT_DEFAULT_ON_EVENT 0
};

typedef struct ctext_config_struct ctext_config

class ctext 
{
  public:
    ctext(WINDOW *win = 0, ctext_config *config = 0);

    //
    // A ctext istance has a configuration specified through
    // the ctext_config structure above
    //
    // When this function is called, a copy of the structure
    // is made so that further modifications are not reflected
    // in a previously instantiated instance.
    //
    int8_t set_config(ctext_config *config);

    //
    // get_config allows you to change a parameter in the 
    // configuration of a ctext instance and to duplicate
    // an existing configuration in a new instance.
    //
    int8_t get_config(ctext_config *config);

    // 
    // At most 1 curses window may be attached at a time.
    //
    // This function specifies the curses window which 
    // will be attached given this instance.
    //
    // If one is already attached, it will be detached and
    // potentially orphaned.
    //
    int8_t attach_curses_window(WINDOW *win);

    //
    // Under normal circumstances, a user would like to
    // remove all the existing content with a clear, starting
    // anew.
    //
    // However, if you'd like to only remove part of the content,
    // then you can pass an amount in and clear will truncate 
    // amount units of the oldest content.
    //
    // The return code is how many rows were cleared from the 
    // buffer.
    //
    size_t clear(size_t amount = 0);

    // 
    // Scroll_to when appending to the bottom in the traditional
    // default sense, will specify the top x and y coordinate 
    // of the viewport.
    //
    // However, if we are appending to the top, then since new
    // content goes above the previous one, scroll_to specifies
    // the lower left coordinate.
    //
    // Returns 0 on success
    //
    int8_t scroll_to(size_t x, size_t y);

    //
    // get_offset returns the current coordinates of the view port.
    // The values from get_offset are complementary to those 
    // of scroll_to
    // 
    int8_t get_offset(size_t*x, size_t*y); 

    // 
    // get_size returns the outer text bounds length (x) and height
    // (y) of the current buffer.
    //
    // More technically speaking, this means the longest row of 
    // content in the buffer for x and the number of rows of content
    // for y.
    //
    int8_t get_size(size_t*x, size_t*y);

    //
    // Each of the directional functions,
    // up, down, left, and right, can be
    // called without an argument to move
    // 1 unit in their respective direction.
    //
    // The return code is how far the movement
    // happened.
    //
    size_t up(size_t amount = 1);
    size_t down(size_t amount = 1);
    size_t left(size_t amount = 1);
    size_t right(size_t amount = 1);

    //
    // printf is identical to printf(3) and can be called
    // from the function at the end of this file, cprintf,
    // with an instance variable.  It places text into the
    // buffer specified.
    //
    // You can take the function pointer of printf from 
    // an existing application and point it to this printf
    // inside of an instance in order to migrate an existing
    // application to this library seamlessly.
    //
    int8_t printf(const char*format, ...);

  private:
    int8_t rebuf();
    int8_t render();
    int8_t direct_scroll(size_t x, size_t y);

    WINDOW *m_win;
    ctext_config m_config;
    ctext_buffer m_buffer;

    size_t m_pos_x;
    size_t m_pos_y;

    size_t m_max_x;
    size_t m_max_y;

    void get_win_size();
    size_t m_win_width;
    size_t m_win_height;
};

int cprintf(ctext*win, const char *format, ...);

#endif
