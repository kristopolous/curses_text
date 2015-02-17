#include "ctext.h"

const ctext_config config_default = {
  .m_tabstop = CTEXT_DEFAULT_TABSTOP,
  .m_scrollback = CTEXT_DEFAULT_SCROLLBACK,
  .m_bounding_box = CTEXT_DEFAULT_BOUNDING_BOX,
  .m_do_wrap = CTEXT_DEFAULT_DO_WRAP,
  .m_append_top = CTEXT_DEFAULT_APPEND_TOP,
  .m_scroll_on_append = CTEXT_DEFAULT_APPEND_TOP,
  .m_on_event = CTEXT_DEFAULT_ON_EVENT
};

ctext::ctext(WINDOW *win = 0, ctext_config *config = 0)
{
  this->m_win = win;
  
  if(config) 
  {
    memcpy(&this->m_config, config, sizeof(ctext_config));
  } 
  else 
  {
    memcpy(&this->m_config, config_default, sizeof(ctext_config));
  }

  this->m_pos_x = 0;
  this->m_pos_y = 0;

  this->m_max_x = 0;
  this->m_max_y = 0;
}

int8_t ctext::set_config(ctext_config *config)
{
  memcpy(&this->m_config, config, sizeof(ctext_config));
  this->render();
  return 0;
}

int8_t ctext::get_config(ctext_config *config)
{
  memcpy(&config, &this->m_config, sizeof(ctext_config));
  return 0;
}

int8_t ctext::attach_curses_window(WINDOW *win)
{
  this->m_win = win;
  return this->render();
}

size_t ctext::clear(size_t amount = 0)
{
  if(amount == 0) 
  {
    this->m_buffer.clear();
  }
  else
  {
    this->m_buffer.erase(this->m_buffer.begin(), this->m_buffer.begin() + amount);
  }
  return this->render();
}

int8_t ctext::scroll_to(size_t x, size_t y)
{
  if(this->m_config.m_bounding_box) 
  {
    x = max(0, x);
    y = max(0, y);
    x = min(x, this->m_max_x);
    y = min(y, this->m_max_y);
  }

  this->m_pos_x = x;
  this->m_pos_y = y;

  return this->render();
}

int8_t ctext::get_offset(size_t*x, size_t*y)
{
  *x = this->m_pos_x;
  *y = this->m_pos_y;
  return 0;
}

int8_t get_size(size_t*x, size_t*y)
{
  *x = this->m_max_x;
  *y = this->m_max_y;
  return 0;
}

int8_t ctext::up(size_t amount = 1) 
{
  return this->down(-amount);
}

int8_t ctext::down(size_t amount = 1) 
{
  return this->scroll_to(this->m_pos_x, this->m_pos_y + amount);
}

int8_t ctext::left(size_t amount = 1) 
{
  return this->right(-amount);
}

int8_t ctext:right(size_t amount = 1) 
{
  return this->scroll_to(this->m_pos_x + amount, this->m_pos_y);
}

int8_t ctext::printf(const char*format, ...) 
{
  this->render();
}

int8_t ctext::printf(const char*format, ...)
{
}

int8_t ctext::render() 
{
}
