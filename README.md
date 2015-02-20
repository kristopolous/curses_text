# ctext

ctext is a small utility class to interact with a curses window analagously to how you would interact
with a screen.

There's an sprintf equivalent that interacts with the object who is associated with a curses WINDOW.

The utility class has a number of parameters which should be familiar:

  * scrollback buffer
  * append on bottom or on top
  * word wrap
  * automatic newline
  * scroll on append

Furthermore it can be manipulated using the following actions:

  * clear
  * up, down, left, right

