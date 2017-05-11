#include"smallwin.h"
#include<gl/gl.h>

int main()
{
   window mainwin = smallwin_open_window("[window title]", 400, 300, 3, 3);

   while(1)
   {
      smallwin_swap_buffers(mainwin);
      if(!non_blocking_message_loop())
         break;

      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(0.3, 0.3, 0.5, 1.0);

   }
   exit_smallwin();

   return 0;
}

