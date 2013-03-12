/* xlossage - display pressed keys in X11 in a readable way */

// gcc -O2 -Wall -o xlossage xlossage.c -lX11 -lXi -g

/* Parts lifted from xorg-xinput:
 *
 * Copyright 1996 by Frederic Lepied, France. <Frederic.Lepied@sugix.frmug.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  the authors  not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     The authors  make  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL THE AUTHORS  BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XI.h>
#include <X11/Xutil.h>

// from Plan9
enum
{
  Bit1    = 7,
  Bitx    = 6,
  Bit2    = 5,
  Bit3    = 4,
  Bit4    = 3,
  
  T1      = ((1<<(Bit1+1))-1) ^ 0xFF,     /* 0000 0000 */
  Tx      = ((1<<(Bitx+1))-1) ^ 0xFF,     /* 1000 0000 */
  T2      = ((1<<(Bit2+1))-1) ^ 0xFF,     /* 1100 0000 */
  T3      = ((1<<(Bit3+1))-1) ^ 0xFF,     /* 1110 0000 */
  T4      = ((1<<(Bit4+1))-1) ^ 0xFF,     /* 1111 0000 */
  
  Rune1   = (1<<(Bit1+0*Bitx))-1,         /* 0000 0000 0111 1111 */
  Rune2   = (1<<(Bit2+1*Bitx))-1,         /* 0000 0111 1111 1111 */
  Rune3   = (1<<(Bit3+2*Bitx))-1,         /* 1111 1111 1111 1111 */
  
  Maskx   = (1<<Bitx)-1,                  /* 0011 1111 */
  Testx   = Maskx ^ 0xFF,                 /* 1100 0000 */
};


void
putkeysym(KeySym keysym, int state)
{
  long c = 0;

  switch (keysym) {
  case XK_Shift_L:
  case XK_Shift_R:
  case XK_Control_L:
  case XK_Control_R:
  case XK_Caps_Lock:
  case XK_Shift_Lock:
  case XK_Meta_L:
  case XK_Meta_R:
  case XK_Alt_L:
  case XK_Alt_R:
  case XK_Super_L:
  case XK_Super_R:
  case XK_Hyper_L:
  case XK_Hyper_R:
  case XK_Mode_switch:
  case XK_ISO_Level3_Shift:
    // ignore
    return;
  }  

  state &= ~8192;  // clear ISO_Group_Shift

  if ((keysym >= 33 && keysym <= 126) || (keysym >= 161 && keysym <= 255)
      || (keysym >= 0x01000100 && keysym <= 0x0110ffff)) {
    // Mapped Latin1 or Unicode
    c = keysym & ~0x1000000;
    if ((state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask)) == ShiftMask)
      state = 0;                // No S- on plain letters.
  }

  if (state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask)) {
    if (state & ShiftMask)   printf("S");
    if (state & ControlMask) printf("C");
    if (state & Mod1Mask)    printf("M");
    if (state & Mod4Mask)    printf("W");
    printf("-");
  }

  if (!c) {
    switch (keysym) {
    case XK_space:     printf("SPC "); break;
    case XK_Return:    printf("RET "); break;
    case XK_BackSpace: printf("DEL "); break;
    default:           printf("%s ", XKeysymToString(keysym));  // Readable name
    }
  } else if (c <= Rune1) {      // Else, generate UTF-8.
    printf("%c ", 
           (char) c);
  } else if (c <= Rune2) {
    printf("%c%c ",
           (char) (T2 | (c >> 1*Bitx)),
           (char) (Tx | (c & Maskx)));
  } else {
    printf("%c%c%c ", (char) (T3 | (c >> 2*Bitx)),
           (char) (Tx | ((c >> 1*Bitx) & Maskx)),
           (char) (Tx | (c & Maskx)));
  }
} 

int
main(int argc, char *argv[])
{
    Display *display;
    int event, error, i, d, xi_opcode;
    int key_press_type = -1;
    long last_time = 0;

    int number = 0;	/* number of events registered */
    XEventClass event_list[7];
    XDevice *device;
    Window root_win;
    unsigned long screen;
    XInputClassInfo *ip;

    XDeviceInfo	*devices;
    int num_devices;
    XEvent Event;
    Atom xi_keyboard;

    display = XOpenDisplay(NULL);

    if (display == NULL) {
      fprintf(stderr, "Unable to connect to X server\n");
      goto out;
    }

    if (!XQueryExtension(display, "XInputExtension",
                         &xi_opcode, &event, &error)) {
      printf("X Input extension not available.\n");
      goto out;
    }

    xi_keyboard = XInternAtom(display, XI_KEYBOARD, 0);

    devices = XListInputDevices(display, &num_devices);

    screen = DefaultScreen(display);
    root_win = RootWindow(display, screen);

    for(d=0; d < num_devices; d++) {
      if (devices[d].type == xi_keyboard) {
        device = XOpenDevice(display, devices[d].id);
        
        for (ip = device->classes, i=0; i<devices->num_classes; ip++, i++)
          if (ip->input_class == KeyClass) {
            DeviceKeyPress(device, key_press_type, event_list[number]);
            number++;
          }
        
        if (XSelectExtensionEvent(display, root_win, event_list, number)) {
          fprintf(stderr, "error selecting extended events\n");
          return 0;
        }
      }
    }

    while(1) {
      XNextEvent(display, &Event);
      
      if (Event.type == key_press_type) {
        XDeviceKeyEvent *key = (XDeviceKeyEvent *) &Event;
        XKeyEvent xevent;
        KeySym keysym;
        char buf[16];

        // Copy XDeviceKeyEvent into an XKeyEvent
        xevent.type = key->type;
        xevent.serial = key->serial;
        xevent.display = key->display;
        xevent.window = key->window;
        xevent.root = key->root;
        xevent.subwindow = key->subwindow;
        xevent.time = key->time;
        xevent.x = key->x;
        xevent.y = key->y;
        xevent.x_root = key->x_root;
        xevent.y_root = key->y_root;
        xevent.state = key->state;
        xevent.keycode = key->keycode;
        xevent.same_screen = key->same_screen;
        
        XLookupString(&xevent, buf, sizeof buf, &keysym, NULL);
        //keysym = XLookupKeysym(&xevent, key->state);
        //keysym = XKeycodeToKeysym(display, key->keycode, 0);

        if (key->time - last_time > 1000)
          printf("\n");
        putkeysym(keysym, key->state);
        fflush(stdout);

        last_time = key->time;
      }
    }
    
    XSync(display, False);
    XCloseDisplay(display);
    
out:
    if (display)
      XCloseDisplay(display);
    return 1;
}
