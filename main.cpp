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
#include <X11/Xutil.h>
#endif
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <curl/curl.h>
#include <curl/easy.h>

//#define SOBEL_FILTER
//#define CANVAS_PINTURILLO

using namespace cv;
typedef Point3_<uint8_t> Pixel;

size_t write_image(void *ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* stream = (FILE*)userdata;
    if (!stream)
    {
        printf("!!! No stream\n");
        return 0;
    }

    size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
    return written;
}

void leftclick()
{
#ifdef _WIN32
  // This structure will be used to create the keyboard input event.
  INPUT Input={0};

  Input.type        = INPUT_MOUSE;									// Let input know we are using the mouse.
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;							// We are setting left mouse button down.
  SendInput(1, &Input, sizeof(INPUT));								// Send the input.

  ZeroMemory(&Input,sizeof(INPUT));									// Fills a block of memory with zeros.
  Input.type        = INPUT_MOUSE;									// Let input know we are using the mouse.
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;								// We are setting left mouse button up.
  SendInput(1, &Input, sizeof(INPUT));
#else
#endif
}

void set_window ()
{
#ifdef _WIN32
   // Find window handle using the window title
   //HWND hWnd = FindWindow(NULL, "Sans titre - Paint");
   HWND hWnd = FindWindow(NULL, "Pinturillo 2 - Pictionary online. Juego chat. Chat game. Draw and guess online game. Juegos Online. Games Online. - Google Chrome");
   if (hWnd)
   {
       // Move to foreground
       SetForegroundWindow(hWnd);
   }
#else
  // Init X windows system
  Display *display = XOpenDisplay(NULL);

  if(display == NULL)
  {
    printf("!!! Cannot initialize the X windows display to set the window in foreground\n");
    return;
  }

  Window root_window = XDefaultRootWindow(display);
  char *window_name = NULL;
/*
  Window wRoot;
  Window wParent;
  Window *wChild = NULL;
  unsigned int nChildren = 0;

  if (XQueryTree(display, root_window, &wRoot, &wParent, &wChild, &nChildren) != 0 )
  {
*/

    Atom a = XInternAtom(display, "_NET_CLIENT_LIST" , true);
    Atom actualType;
    int format;
    unsigned long numItems, bytesAfter;
    unsigned char *data = 0;
    int status = XGetWindowProperty(display,
                                RootWindow(display, DefaultScreen(display)),
                                a,
                                0L,
                                (~0L),
                                false,
                                AnyPropertyType,
                                &actualType,
                                &format,
                                &numItems,
                                &bytesAfter,
                                &data);

sleep(3);
  if (status == Success)
  {
    unsigned int *window_array = (unsigned int *) data;
    for (unsigned int i = 0; i < numItems; i++)
    {
      Window w = (Window) window_array[i];
      printf("%lu ", w);
      if (XFetchName(display, w, &window_name) != 0 && window_name != NULL)
      {
        printf("Window %d name %s\n", i, window_name);
        XFree(window_name);
      }
    }

    XFree(data);
  }

return;
#endif
}

void draw_image (Mat *image, int thresh)
{
#ifdef _WIN32
  // This structure will be used to create the keyboard input event.
  INPUT Input={0};
#else
  // Init X windows system
  Display *display = XOpenDisplay(NULL);

  if(display == NULL)
  {
    printf("!!! Cannot initialize the X windows display to move the mouse\n");
    return;
  }

  Window root_window = DefaultRootWindow(display);
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
           sleep(50);
#ifdef _WIN32
           SetCursorPos(550+j, 400+i);

           Input.type        = INPUT_MOUSE;									// Let input know we are using the mouse.
           Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;							// We are setting left mouse button down.
           SendInput(1, &Input, sizeof(INPUT));								// Send the input.
#else
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, 550+j, 400+i);

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

    if(XSendEvent(display, PointerWindow, true, 0xfff, &event) == 0) fprintf(stderr, "Error\n");

    XFlush(display);
#endif

           while((static_cast<int>(image->at<uchar>(i,j)) < thresh) && j < image->cols)
           {
              image->at<uchar>(i,j) = 0;
              j++;
           }

#ifdef _WIN32
           sleep(50);
           SetCursorPos(550+j, 400+i);
           ZeroMemory(&Input,sizeof(INPUT));									// Fills a block of memory with zeros.
           Input.type        = INPUT_MOUSE;									// Let input know we are using the mouse.
           Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;								// We are setting left mouse button up.
           SendInput(1, &Input, sizeof(INPUT));
#else
    event.type = ButtonRelease;
    event.xbutton.state = 0x100;

    if(XSendEvent(display, PointerWindow, true, 0xfff, &event) == 0) fprintf(stderr, "Error\n");

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
#ifndef _WIN32
    XCloseDisplay(display);
#endif
}

int main(int argc, char *argv[])
{
    CURL* curl;
    CURLcode res;
    char* url;// = "http://www.personal.psu.edu/jyc5774/jpg.jpg";
set_window();
    if(argc < 2 || argc > 2)
    {
        printf("Program needs exactly 1 arg (URL of image) %d \n", argc);
        return -1;
    }

    // Copy argument into URL
    url = argv[1];

    FILE* fp = fopen("test.png", "wb");
    if (!fp)
    {
        printf("!!! Failed to create file on the disk\n");
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
        printf("!!! Failed to download: %s %d\n", url, rc);
        return -1;
    }

    long res_code = 0;
    curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
    if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
    {
        printf("!!! Response code: %d\n", res_code);
        return -1;
    }

    curl_easy_cleanup(curlCtx);
    fclose(fp);

    Mat original_image;
    Mat gray_image;
    // Lit l'image
    //original_image = cv::imread("test.png", 1);
    gray_image = cv::imread("test.png", IMREAD_GRAYSCALE);

    //cvtColor(original_image, gray_image, CV_BGR2GRAY);

    // Sortie seuillage
    Mat bw_image;

    // Sortie redimensionnement
    Mat bw_resize_image;

    // Taille de sortie
    Size canvas_size;
    canvas_size.width = 128;
    canvas_size.height = 128;

    // Valeurs de seuillage et max
    int thresh = 100;
    int max_thresh = 255;

    // Valeurs d'échelle, profondeur et delta pour le calcul de gradient (sobel)
    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;

    // Gradient de x, gradient de y, gradient total (Sobel)
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;
    Mat grad;

#ifdef SOBEL_FILTER
    // Calcul de gradient en X
    Sobel(gray_image, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(grad_x, abs_grad_x);

    // Calcul de gradient en Y
    Sobel(gray_image, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(grad_y, abs_grad_y);

    // Gradient total (approximation)
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
    // Voir http://docs.opencv.org/doc/tutorials/imgproc/imgtrans/sobel_derivatives/sobel_derivatives.html

    // Seuillage
    threshold(grad, bw_image, thresh, max_thresh, THRESH_BINARY);
#endif

/*
    1 point = 6 (longueur) x 4 (largeur) pixels
    canvas = ~ 500 x 500 pixels
*/

    // Redimensionnement
    resize(gray_image, bw_resize_image, canvas_size, 0, 0, INTER_AREA);


  // If the parameter is available, set the window specified by the user in foreground
  set_window();

    // Pause for 5 seconds.
    sleep(2000);

//    SetCursorPos(200, 200);

#ifdef CANVAS_PINTURILLO
    for(int i = 0; i < bw_resize_image.rows; i++){
        for(int j = 0; j < bw_resize_image.cols; j++){
            SetCursorPos(550+j, 400+i);
            //SetCursorPos((rect.right - 200) + j, (rect.bottom - 200) + i);
/*
            Vec3b intensity = bw_resize_image.at<Vec3b>(250+j, 250+i);
            if(intensity[0] == 255 && intensity[1] == 255 && intensity[2] == 255)
            {
                leftclick();
            }
*/
           // printf("%d,%d,%d ", intensity[0], intensity[1], intensity[2]);

/*
            // Si pixel noir
            if(bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 0] == 0 &&
                bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 1] == 0 &&
                bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 2] == 0)
              {
                  leftclick();
              }


            // Si pixel blanc
            if(bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 0] == 255 &&
                bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 1] == 255 &&
                bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 2] == 255)
            {
                leftclick();
            }
*/
            //unsigned char * p = bw_resize_image.ptr(j, i);
            //std::cout << int(bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 0])
            //    << "," << int(bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 1])
            //    << "," << int(bw_resize_image.data[bw_resize_image.step[0]*i + bw_resize_image.step[1]* j + 2]);
            //std::cout << static_cast<int>(p[0]) << "," << static_cast<int>(p[1]) << "," << static_cast<int>(p[2]) << " ";//bw_resize_image.at<int>(i,j) << " ";
            //std::cout << static_cast<int>(bw_resize_image.at<uchar>(i,j)) << " # ";
            if(static_cast<int>(bw_resize_image.at<uchar>(i,j)) < thresh)
            {
                leftclick();
                bw_resize_image.at<uchar>(i,j) = 0;
            }
            else
            {
                bw_resize_image.at<uchar>(i,j) = 255;
            }
        }
        //std::cout << ""<< std::endl;
    }
#endif

  // Convert image to mouse inputs
  draw_image(&bw_resize_image, thresh);

    sleep(3000);
    namedWindow("Display window", CV_WINDOW_AUTOSIZE);// create a window for display.
    imshow("Display window", bw_resize_image);// show our image inside it.
    waitKey(0);// wait for a keystroke in the window

    return 0;
}
