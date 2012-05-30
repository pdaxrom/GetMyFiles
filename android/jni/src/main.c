/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "client.h"

#define  LOG_TAG    "getmyfiles"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define MAX_CLIENTS	10

static client_args *clients[MAX_CLIENTS];

static pthread_mutex_t mutex_c;

static void *thread_client(void *arg)
{
    client_args *c = (client_args *) arg;
    client_connect(c);

    pthread_mutex_lock(&mutex_c);
    if (clients[c->id])
	clients[c->id] = NULL;
    free((void *)c->root_dir);
    free(c);
    pthread_mutex_unlock(&mutex_c);

    return NULL;
}


void
Java_com_getmyfiles_client_MainActivity_init(
    JNIEnv*  env,
    jobject  this
				    )
{
    memset(clients, 0, sizeof(clients));
    pthread_mutex_init(&mutex_c, NULL);
}

void
Java_com_getmyfiles_client_MainActivity_fini(
    JNIEnv*  env,
    jobject  this
				    )
{
    pthread_mutex_destroy(&mutex_c);
}


jint
Java_com_getmyfiles_client_MainActivity_connect(
    JNIEnv*  env,
    jobject  this,
    jstring  root_dir,
    jint     enable_httpd,
    jint     max_ext_conn,
    jint     max_int_conn
					)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
	if (!clients[i]) {
	    pthread_t tid;
	    const char *dir = (*env)->GetStringUTFChars(env, root_dir, 0);
	    clients[i] = malloc(sizeof(client_args));
	    clients[i]->max_ext_conns = max_ext_conn;
	    clients[i]->max_int_conns = max_int_conn;
	    clients[i]->enable_httpd = enable_httpd;
	    clients[i]->exit_request = 0;
	    clients[i]->host = "getmyfil.es";
	    clients[i]->port = 8100;
	    clients[i]->root_dir = strdup(dir);
	    clients[i]->id = i;
	    (*env)->ReleaseStringUTFChars(env, root_dir, dir);

	    if (pthread_create(&tid, NULL, &thread_client, (void *) &clients[i]) != 0) {
//		fprintf(stderr, "Can't create client's thread\n");
		LOGE("Can't create client's thread");
		free((void *)clients[i]->root_dir);
		free(clients[i]);
		clients[i] = NULL;
		return -1;
	    }

	    return i;
	}
    }
    return -1;
}

void
Java_com_getmyfiles_client_MainActivity_disconnect(
    JNIEnv*  env,
    jobject  this,
    jint     id
					)
{
    pthread_mutex_lock(&mutex_c);
    if (clients[id]) {
	clients[id]->exit_request = 1;
	clients[id] = NULL;
    }
    pthread_mutex_unlock(&mutex_c);
}

void show_server_directory(int id, char *str)
{
    jstring jstr = (*env)->NewStringUTF(env, str);
    jclass clazz = (*env)->FindClass(env, "com/getmyfiles/client/MainActivity");
    jmethodID messageMe = (*env)->GetMethodID(env, clazz, "showUrl", "(ILjava/lang/String;)");
    (*env)->CallObjectMethod(env, obj, messageMe, id, jstr);
    //
}

void update_client(int id, char *vers)
{
    jstring jstr = (*env)->NewStringUTF(env, vers);
    jclass clazz = (*env)->FindClass(env, "com/getmyfiles/client/MainActivity");
    jmethodID messageMe = (*env)->GetMethodID(env, clazz, "showUpdate", "(ILjava/lang/String;)");
    (*env)->CallObjectMethod(env, obj, messageMe, id, jstr);
    //
}
