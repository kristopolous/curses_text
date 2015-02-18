#include "ctext.h"
#include <string.h>
#include <algorithm>    // std::max

using namespace std;

const ctext_config config_default = {
  .m_scrollback = CTEXT_DEFAULT_SCROLLBACK,
  .m_bounding_box = CTEXT_DEFAULT_BOUNDING_BOX,
  .m_do_wrap = CTEXT_DEFAULT_DO_WRAP,
  .m_append_top = CTEXT_DEFAULT_APPEND_TOP,
  .m_scroll_on_append = CTEXT_DEFAULT_APPEND_TOP,
  .m_on_event = CTEXT_DEFAULT_ON_EVENT
};

char large_buffer[8192] = {0};

ctext::ctext(WINDOW *win, ctext_config *config)
{
  this->m_win = win;
  
  if(config) 
  {
    memcpy(&this->m_config, config, sizeof(ctext_config));
  } 
  else 
  {
    memcpy(&this->m_config, &config_default, sizeof(ctext_config));
  }

  this->m_pos_x = 0;
  this->m_pos_y = 0;

  this->m_max_x = 0;
  this->m_max_y = 0;
}

int8_t ctext::set_config(ctext_config *config)
{
  memcpy(&this->m_config, config, sizeof(ctext_config));
  return this->render();
}

int8_t ctext::get_config(ctext_config *config)
{
  memcpy(config, &this->m_config, sizeof(ctext_config));
  return 0;
}

int8_t ctext::attach_curses_window(WINDOW *win)
{
  this->m_win = win;
  return this->render();
}

int16_t ctext::clear(int16_t amount)
{
  int16_t ret = 0;
  if(amount == 0) 
  {
    ret = this->m_buffer.size();
    this->m_buffer.clear();
  }
  else
  {
    if(this->m_buffer.size()) 
    {
      ret = this->m_buffer.size();
      this->m_buffer.erase(this->m_buffer.begin(), this->m_buffer.begin() + amount);
      ret -= this->m_buffer.size();
    }
  }

  if (this->m_config.m_on_event)
  {
    this->m_config.m_on_event(this, CTEXT_CLEAR);
  }

  this->render();
  return ret;
}

int8_t ctext::direct_scroll(int16_t x, int16_t y)
{
  if(this->m_config.m_bounding_box) 
  {
    x = max(0, (int32_t)x);
    y = max(0, (int32_t)y);
    x = min(x, this->m_max_x);
    y = min(y, this->m_max_y);
  }

  this->m_pos_x = x;
  this->m_pos_y = y;

  if (this->m_config.m_on_event)
  {
    this->m_config.m_on_event(this, CTEXT_SCROLL);
  }

  return 0;
}

int8_t ctext::scroll_to(int16_t x, int16_t y)
{
  this->direct_scroll(x, y);
  return this->render();
}

int8_t ctext::get_offset(int16_t*x, int16_t*y)
{
  *x = this->m_pos_x;
  *y = this->m_pos_y;

  return 0;
}

int8_t ctext::get_size(int16_t*x, int16_t*y)
{
  *x = this->m_max_x;
  *y = this->m_max_y;

  return 0;
}

int16_t ctext::up(int16_t amount) 
{
  return this->down(-amount);
}

int16_t ctext::down(int16_t amount) 
{
  return this->scroll_to(this->m_pos_x, this->m_pos_y + amount);
}

int16_t ctext::left(int16_t amount) 
{
  return this->right(-amount);
}

int16_t ctext::right(int16_t amount) 
{
  return this->scroll_to(this->m_pos_x + amount, this->m_pos_y);
}

void ctext::get_win_size() 
{
  int32_t width = 0, height = 0;

  if(this->m_win)
  {
    getmaxyx(this->m_win, height, width);
  }
  this->m_win_width = width;
  this->m_win_height = height;
}

int8_t ctext::rebuf()
{
  this->get_win_size();

  // The actual scroll back, which is what
  // people expect in this feature is the 
  // configured scrollback + the window height.
  int16_t actual_scroll_back = this->m_win_height + this->m_config.m_scrollback;

  if(this->m_buffer.size() > actual_scroll_back)
  {
    this->m_buffer.erase(this->m_buffer.begin(), this->m_buffer.end() - actual_scroll_back);
  }
  
  this->m_max_x = 0;  
  //
  // Now unfortunately we have to do a scan over everything in N time to find
  // the maximum length string --- but only if we care about the bounding
  // box
  //
  if(this->m_config.m_bounding_box)
  {
    for(ctext_buffer::const_iterator it = this->m_buffer.begin(); it != this->m_buffer.end(); it++) 
    {
      this->m_max_x = max((int)this->m_max_x, (int)(*it).size());
    }
  }
 
  // this is practically free so we'll just do it.
  this->m_max_y = this->m_buffer.size();
  
  //
  // Since we've changed the bounding box of the content we have to
  // issue a rescroll on exactly our previous parameters. This may
  // force us inward or may retain our position.
  // 
  return this->direct_scroll(this->m_pos_x, this->m_pos_y);
}

int cprintf(ctext*win, const char *format, ...)
{
  int ret;
  va_list args;
  va_start(args, format);
  ret = win->vprintf(format, args);
  va_end(args);
  return ret;
}

int8_t ctext::vprintf(const char*format, va_list ap)
{
  memset(large_buffer, 0, sizeof(large_buffer));
  vsprintf(large_buffer, format, ap);

  wstring wstr (large_buffer, large_buffer + strlen(large_buffer));
  this->m_buffer.push_back(wstr);

  if (this->m_config.m_on_event)
  {
    this->m_config.m_on_event(this, CTEXT_DATA);
  }
  
  // Since we are adding content we need to see if we are
  // to force on scroll.
  if(this->m_config.m_scroll_on_append)
  {
    this->get_win_size();
    // now we force it.
    this->direct_scroll(0, this->m_buffer.size() - this->m_win_height);
  }

  return this->render();
}

int8_t ctext::printf(const char*format, ...)
{
  int8_t ret;

  va_list args;
  va_start(args, format);
  ret = this->vprintf(format, args);

  va_end(args);
  return ret;
}

int8_t ctext::render() 
{
  // Calculate the bounds of everything first.
  this->rebuf();

  if(!this->m_win)
  {
    // Not doing anything without a window.
    return -1;
  }

  this->get_win_size();

  //
  // By this time, if we are bounded by a box,
  // it has been accounted for.
  //
  // Really our only point of interest is
  // whether we need to append to bottom
  // or append to top.
  //
  // We will assume that we can
  // populate the window quick enough
  // to avoid linear updating or paging.
  //  ... it's 2015 after all.
  //
  werase(this->m_win);

  // Regardless of whether this is append to top
  // or bottom we generate top to bottom.

  int16_t start_char = max(0, (int32_t)this->m_pos_x);
  int16_t offset = start_char;
  // the endchar will be in the substr
  
  //
  // We start as m_pos_y in our list and move up to
  // m_pos_y + m_win_height except in the case of 
  // wrap around.  Because of this special case,
  // we compute when to exit slightly differently.
  //
  // This is the current line of output, which stays
  // below m_win_height
  //
  int16_t line = 0;

  // start at the beginning of the buffer.
  int16_t index = this->m_pos_y;
  int16_t directionality = +1;
  wstring to_add;
  wstring *source;

  // if we are appending to the top then we start
  // at the end and change our directionality.
  if(this->m_config.m_append_top)
  {
    directionality = -1;
    index = this->m_pos_y + this->m_win_height - 1;
  }

  while(line <= this->m_win_height)
  {
    // Reset the offset.
    offset = start_char;

    if(index < this->m_max_y && index >= 0)
    {
      // We only index into the object if we have the
      // data to do so.
      source = &this->m_buffer[index];
      to_add = (*source).substr(offset, this->m_win_width);
      mvwaddwstr(this->m_win, line, 0, to_add.c_str());

      // if we are wrapping, then we do that here.
      while(
          this->m_config.m_do_wrap && 

          // if our string still exhausts our entire width
          to_add.size() == this->m_win_width &&

          // and we haven't hit the bottom
          line <= this->m_win_height
        )
      {
        // move our line forward
        line++;

        // and the start_char
        offset += this->m_win_width;

        // substring into this character now at this advanced position
        to_add = this->m_buffer[index].substr(offset, this->m_win_width);
        
        // and add it to the screen
        mvwaddwstr(this->m_win, line, 0, to_add.c_str());
      }
    }
    index += directionality;
    line++;
  }

  wrefresh(this->m_win);
}
