#include <iostream>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x)
#else
#include <time.h>
#include <unistd.h>
#define sleep(x) usleep(x*1000)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif
#include <getopt.h>
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <curl/curl.h>
#include <curl/easy.h>

using namespace cv;
typedef Point3_<uint8_t> Pixel;

size_t write_image(void *ptr, size_t size, size_t nmemb, void* userdata)
{
  FILE* stream = (FILE*)userdata;
  if (!stream)
  {
    printf("No stream error \n");
    return 0;
  }

  size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
  return written;
}

Window set_window (Display *display, char *window_name)
{
#ifdef _WIN32
  // Find window handle using the window title
  HWND hWnd = FindWindow(NULL, window_name);
  if (hWnd)
  {
    // Move to foreground
    SetForegroundWindow(hWnd);
  }
#else
  Atom prop = XInternAtom(display,"_NET_CLIENT_LIST",False);
  Atom type;
  int form;
  unsigned long remain;
  unsigned char *list;
  Window *list_windows;
  unsigned long len;
  int i;

  if (XGetWindowProperty(display,XDefaultRootWindow(display),prop,0,1024,False,XA_WINDOW,
        &type,&form,&len,&remain,&list) != Success)
  {
    printf("Get list of windows error \n");
    return -1;
  }

  list_windows = (Window *)list;

  for (i = 0; i < (int)len; i++)
  {
    Atom prop_name = XInternAtom(display,"WM_NAME",False);
    unsigned long len_name;
    unsigned char *list_names;

    if (XGetWindowProperty(display,list_windows[i],prop_name,0,1024,False,XA_STRING,
          &type,&form,&len_name,&remain,&list_names) != Success)
    {
      printf("Get name of window error \n");
      return -1;
    }

    printf("Window name %s\n", (char *)list_names);

    if (strstr ((char *)list_names, window_name) != NULL)
    {
      //    XSetInputFocus(display, list_windows[i], RevertToPointerRoot, CurrentTime);
      if (!XRaiseWindow(display, list_windows[i]))
      {
        printf("Raise window error \n");
        return -1;
      }

      //  XSync(display, false);
      XFlush(display);
      break;
    }
  }

  return list_windows[i];
#endif
}

void draw_image (Display *display, Mat *image, int thresh, Window window)
{
#ifdef _WIN32
  // This structure will be used to create the keyboard input event.
  INPUT Input={0};
#else
  XEvent event;
#endif

  // Do thresholding and drawing at the same time
  // Draw line by line
  for(int i = 0; i < image->rows; i++)
  {
    int j = 0;
    while(j < image->cols)
    {
      if(static_cast<int>(image->at<uchar>(i,j)) < thresh)
      {
        // Sleep 50ms to let actualize
        sleep(50);
#ifdef _WIN32
        // Set position
        SetCursorPos(550+j, 400+i);

        // Press down left click on mouse
        Input.type        = INPUT_MOUSE;
        Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &Input, sizeof(INPUT));
#else
        XWarpPointer(display, None, window, 0, 0, 0, 0, 550+j, 400+i);

        memset(&event, 0x00, sizeof(event));

        event.type = ButtonPress;
        event.xbutton.button = Button1;
        event.xbutton.same_screen = true;

        XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

        event.xbutton.subwindow = event.xbutton.window;

        while(event.xbutton.subwindow)
        {
          event.xbutton.window = event.xbutton.subwindow;

          XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
        }

        if(XSendEvent(display, PointerWindow, true, 0xfff, &event) == 0) {
          printf("Press left mouse button error \n");
        }

        // Flush to apply changes
        XFlush(display);
#endif

        while((static_cast<int>(image->at<uchar>(i,j)) < thresh) && j < image->cols)
        {
          image->at<uchar>(i,j) = 0;
          j++;
        }

        // Sleep 50ms to let actualize
        sleep(50);
#ifdef _WIN32
        SetCursorPos(550+j, 400+i);

        // Release left click on mouse
        ZeroMemory(&Input,sizeof(INPUT));
        Input.type        = INPUT_MOUSE;
        Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
        SendInput(1, &Input, sizeof(INPUT));
#else
        XWarpPointer(display, None, window, 0, 0, 0, 0, 550+j, 400+i);

        event.type = ButtonRelease;
        event.xbutton.state = 0x100;

        if(XSendEvent(display, PointerWindow, true, 0xfff, &event) == 0) {
          printf("Release left mouse button error \n");
        }

        // Actualize
        XFlush(display);
#endif

        //image->at<uchar>(i,j) = 0;
      }
      else
      {
        image->at<uchar>(i,j) = 255;
      }

      j++;
    }
  }
}

void print_usage() {
  printf("Usage : (URL mandatory) image_to_mouse [--sobel] [hwld] -u <URL>\n");
}

int main(int argc, char *argv[])
{
  CURL* curl;
  CURLcode res;

  // URL of image to draw
  char* url = NULL;// = "http://www.personal.psu.edu/jyc5774/jpg.jpg";

  // Title of window to use for drawing
  char *window_title = NULL; //"[Not Saved]";

  // Size of output image
  Size canvas_size;
  canvas_size.width = 128;
  canvas_size.height = 128;

  //  Thresholding values
  int thresh = 100;
  int max_thresh = 255;

  char opt = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h' },
      {"sobel", no_argument, 0, 's' },
      {"width", optional_argument, 0, 'w' },
      {"length", optional_argument, 0, 'l' },
      {"window", optional_argument, 0, 'd' },
      {"url", required_argument, 0, 'u' },
      {NULL, 0, NULL,  0 }
  };

  opt = getopt_long(argc, argv,"hswldu:", long_options, NULL);
  if (opt == -1) {
    print_usage();
    return -1;
  }

  do {
    switch (opt) {
      case 's':
        #define SOBEL_FILTER
        break;
      case 'w':
        canvas_size.width = atoi(optarg);
        break;
      case 'l':
        canvas_size.height = atoi(optarg);
        break;
      case 'u':
        // Copy argument into URL
        url = optarg;
        break;
      case 'd':
        // Copy argument into window name
        window_title = optarg;
        break;
      case 'h':
      case ':':
      case '?':
      default:
        print_usage();
        return -1;
    }
  } while ((opt = getopt_long(argc, argv,"hswldu:", long_options, NULL)) != -1);

  if (url == NULL) {
    printf("Missing url \n");
    print_usage();
  }

  // Init X windows system
  Display *display = XOpenDisplay(NULL);
  if(display == NULL)
  {
    printf("Cannot initialize the X windows display \n");
    return -1;
  }


  FILE* fp = fopen("test.png", "wb");
  if (!fp)
  {
    printf("Failed to create file on the disk\n");
    return -1;
  }

  CURL* curlCtx = curl_easy_init();
  curl_easy_setopt(curlCtx, CURLOPT_URL, url);
  curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, write_image);
  curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlCtx, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curlCtx, CURLOPT_SSL_VERIFYHOST, 0L);

  CURLcode rc = curl_easy_perform(curlCtx);
  if (rc)
  {
    printf("Failed to download: %s %d\n", url, rc);
    return -1;
  }

  long res_code = 0;
  curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
  if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
  {
    printf("CURL response code: %d\n", res_code);
    return -1;
  }

  curl_easy_cleanup(curlCtx);
  fclose(fp);

  Mat original_image;
  Mat gray_image;
  // Read pic downloaded
  gray_image = cv::imread("test.png", IMREAD_GRAYSCALE);

  // Threshold output matrix
  Mat bw_image;

  // Resizing output matrix
  Mat bw_resize_image;

#ifdef SOBEL_FILTER
  // Sobel vars 
  int scale = 1;
  int delta = 0;
  int ddepth = CV_16S;

  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;
  Mat grad;

  // Computing X gradient
  Sobel(gray_image, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
  convertScaleAbs(grad_x, abs_grad_x);

  // Computing Y gradient
  Sobel(gray_image, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
  convertScaleAbs(grad_y, abs_grad_y);

  // Total gradient (approximated)
  addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
  // See http://docs.opencv.org/doc/tutorials/imgproc/imgtrans/sobel_derivatives/sobel_derivatives.html

  // Thresholding
  threshold(grad, bw_image, thresh, max_thresh, THRESH_BINARY);
#endif

  /*
    1 point = 6 (length) x 4 (width) pixels
    canvas = ~ 500 x 500 pixels
  */

  // Resizing
  resize(gray_image, bw_resize_image, canvas_size, 0, 0, INTER_AREA);

  Window drawing_window;
  if (window_title != NULL) {
    // If the parameter is available, set the window specified by the user in foreground
    drawing_window = set_window(display, window_title);
  } else {
    drawing_window = DefaultRootWindow(display);
  }

  // Pause for 2 seconds.
  sleep(2000);

  // Convert image to mouse inputs
  draw_image(display, &bw_resize_image, thresh, drawing_window);

#ifndef _WIN32
  // Close X connection
  XCloseDisplay(display);
#endif

  // Pause for 3 seconds
  sleep(3000);

  // Display image drawn with the mouse in an opencv window (debug purposes)
  namedWindow("Display window", CV_WINDOW_AUTOSIZE);
  imshow("Display window", bw_resize_image);
  // Wait for a keystroke in the opencv window to exit
  waitKey(0);

  return 0;
}
