#include <gui.h>
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
#include <pthread.h>
#else
#include <windows.h>
#include <process.h>
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
#include <FL/x.H>
#include <X11/xpm.h>
#include "icon/icon.xpm"
#endif
#if defined(_WIN32)
#include <FL/x.H>
#include "resource.h"
#endif
#include <FL/fl_ask.H>
#include "client.h"

char infoText[128];

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
pthread_t tid;
#endif

static int sock;

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
static void *thread_client(void *arg)
#else
static void thread_client(void *arg)
#endif
{
    char *host = "getmyfil.es";
    int port = 8100;

    set_online();
    client_connect(host, port, (char *) arg, &sock);
    set_offline();

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
    return NULL;
#endif
}

int online_client(const char *path)
{
    fprintf(stderr, "Path %s\n", path);

    if (strlen(path) == 0) {
	set_offline();
	return 0;
    }

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
    if (pthread_create(&tid, NULL, &thread_client, (void *) path) != 0) {
#else
    if ((int)_beginthread(thread_client, 0, (VOID *) path) == -1) {
#endif
	fprintf(stderr, "Can't create client's thread\n");
	return 0;
    }

    return 1;
}

int offline_client(void)
{
    client_disconnect(sock);
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
    pthread_cancel(tid);
#endif
    return 0;
}

void update_client(char *vers)
{
    snprintf(infoText, sizeof(infoText), "Please update this client to version %s or better!", vers);
    infoStr->label(infoText);
#ifdef _WIN32
    fl_message(infoText);
#endif
}

int main(int argc, char *argv[])
{
    Fl_Double_Window *w = make_window();
//    w->show(argc, argv);

    snprintf(infoText, sizeof(infoText), "GetMyFil.es client version %.02f", VERSION);
    infoStr->label(infoText);

#ifndef __APPLE__
    if (argc > 1) {
	show_shared_directory(argv[1]);
	online_client(argv[1]);
    }
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
    fl_open_display();

    Pixmap p, mask;
    XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display), icon_xpm, &p, &mask, NULL);

    w->icon((char *)p);
#endif
#if defined(_WIN32)
    w->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#endif

    w->show();

    return Fl::run();
}
