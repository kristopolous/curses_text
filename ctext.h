#include <string>
#include <vector>
#include <ncursesw/ncurses.h>
#include <stdint.h>

#ifndef __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT
#define __83a9222a_c8b9_4f36_9721_5dfbaccb28d0_CTEXT
#define CTEXT_BUFFER_SIZE (8192)

using namespace std;

typedef enum ctext_event_enum {
  CTEXT_SCROLL,
  CTEXT_CLEAR,
  CTEXT_DATA,
  CTEXT_CONFIG
} ctext_event;

class ctext;

struct ctext_config_struct
{
  //
  // This specifies how many lines are kept
  // in the ring-buffer.  
  //
  int16_t m_buffer_size;
#define CTEXT_DEFAULT_BUFFER_SIZE 100

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
#define CTEXT_DEFAULT_SCROLL_ON_APPEND false

  //
  // The auto_newline boolean will specify whether
  // a newline is appended at the end of every printf
  // call automatically for you ... as opposed to
  // the more traditional printf where it is not.
  //
  // Our definition of newline is unixes, that is
  // the single character of 0x0A, or \n.
  //
  bool m_auto_newline;
#define CTEXT_DEFAULT_AUTO_NEWLINE false

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
  int8_t (*m_on_event)(ctext *context, ctext_event event);
#define CTEXT_DEFAULT_ON_EVENT 0
};

typedef struct ctext_config_struct ctext_config;

typedef struct ctext_format_struct
{
  int32_t offset;
  attr_t attrs;
  int16_t color_pair;
} ctext_format;

typedef struct ctext_row_struct
{
  wstring data;
  vector<ctext_format> format;
} ctext_row;

typedef vector<ctext_row> ctext_buffer;

class ctext 
{
  public:
    ctext(WINDOW *win = 0, ctext_config *config = 0);

    //
    // A ctext instance has a configuration specified through
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
    int16_t clear(int16_t amount = 0);

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
    int8_t scroll_to(int16_t x, int16_t y);

    //
    // get_offset returns the current coordinates of the view port.
    // The values from get_offset are complementary to those 
    // of scroll_to
    // 
    int8_t get_offset(int16_t*x, int16_t*y); 

    // 
    // get_size returns the outer text bounds length (x) and height
    // (y) of the current buffer.
    //
    // More technically speaking, this means the longest row of 
    // content in the buffer for x and the number of rows of content
    // for y.
    //
    int8_t get_size(int16_t*x, int16_t*y);

    //
    // Each of the directional functions,
    // up, down, left, and right, can be
    // called without an argument to move
    // 1 unit in their respective direction.
    //
    // The return code is how far the movement
    // happened.
    //
    int16_t up(int16_t amount = 1);
    int16_t down(int16_t amount = 1);
    int16_t left(int16_t amount = 1);
    int16_t right(int16_t amount = 1);

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
    int32_t putchar(int32_t c);
    int8_t printf(const char*format, ...);
    int8_t vprintf(const char*format, va_list ap = 0);

  private:
    void add_row();
    int8_t rebuf();
    int8_t render();
    int8_t direct_scroll(int16_t x, int16_t y);

    WINDOW *m_win;
    ctext_config m_config;
    ctext_buffer m_buffer;

    int16_t m_pos_x;
    int16_t m_pos_y;

    int16_t m_max_x;
    int16_t m_max_y;

    void get_win_size();
    int16_t m_win_width;
    int16_t m_win_height;
};

int cprintf(ctext*win, const char *format, ...);

#endif
