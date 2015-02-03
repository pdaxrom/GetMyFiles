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
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "technology_madisa_getmyfiles_External.h"
#include "client.h"

#define  LOG_TAG    "getmyfiles"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static client_args client;

static void *thread_client(void *arg)
{
//    set_online();
    client_connect((client_args *) arg);
//    set_offline();

    return NULL;
}


JNIEXPORT jint JNICALL Java_technology_madisa_getmyfiles_External_connectClient
  (JNIEnv *env, jobject this, jobjectArray stringArray)
{
    pthread_t tid;

    int stringCount = (*env)->GetArrayLength(env, stringArray);

    int i = 0;
    jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
    const char *path = (*env)->GetStringUTFChars(env, string, NULL);

    client.enable_httpd = 1;
    client.max_ext_conns = 10;
    client.max_int_conns = 10;
    client.host = "getmyfil.es";
    client.port = 8100;
    client.root_dir = strdup(path);
    client.exit_request = 0;
    client.id = 0;

    if (pthread_create(&tid, NULL, &thread_client, (void *) &client) != 0) {
	fprintf(stderr, "Can't create client's thread\n");
	return 1;
    }

    (*env)->ReleaseStringUTFChars(env, string, path);

    return 0;
}

void show_server_directory(int id, char *str)
{
}

void update_client(int id, char *vers)
{
}

#if 0
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
#endif
