#include <gui.h>
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
#include <pthread.h>
#else
#include <windows.h>
#include <process.h>
#endif
#include "client.h"

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
    char *host = "itsmyfiles.com";
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

int main(int argc, char *argv[])
{
    Fl_Double_Window *w = make_window();
//    w->show(argc, argv);

#ifndef __APPLE__
    if (argc > 1) {
	show_shared_directory(argv[1]);
	online_client(argv[1]);
    }
#endif

    w->show();

    return Fl::run();
}
