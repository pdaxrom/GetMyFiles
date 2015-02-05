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
#include "com_getmyfiles_gui_GetMyFilesActivity.h"
#include "client.h"

#define  LOG_TAG    "getmyfiles"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static client_args client;

typedef struct {
    JNIEnv	*env;
    jobject	this;
    jclass	clazz;

    jmethodID	showServerDirectory;
    jmethodID	updateClient;
} JavaVars;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return -1;

    LOGI("JNI INIT");

    return JNI_VERSION_1_6;
}

JNIEXPORT jint JNICALL Java_com_getmyfiles_gui_GetMyFilesActivity_connectClient
  (JNIEnv *env, jobject this, jobjectArray stringArray)
{
    int status = 0;

    LOGI("JNI work !");

    int stringCount = (*env)->GetArrayLength(env, stringArray);

    int i = 0;
    jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
    const char *path = (*env)->GetStringUTFChars(env, string, NULL);

    client.enable_httpd = 1;
    client.max_ext_conns = 10;
    client.max_int_conns = 10;
    client.host = "getmyfil.es";
    client.port = 8100;
    client.root_dir = path;
    client.exit_request = 0;
    client.priv = malloc(sizeof(JavaVars));

    JavaVars *java = (JavaVars *)client.priv;

    java->env = env;
    java->this = this;
    java->clazz = (*env)->FindClass(env, "com/getmyfiles/gui/GetMyFilesActivity");
    if (java->clazz == 0) {
        LOGI("FindClass error");
	status = 1;
	goto err1;
    }
    java->showServerDirectory = (*env)->GetMethodID(env, java->clazz, "showServerDirectory", "(Ljava/lang/String;)V");
    if (java->showServerDirectory == 0) {
        LOGI("GetMethodID error - showServerDirectory");
	status = 1;
        goto err1;
    }
    java->updateClient = (*env)->GetMethodID(env, java->clazz, "updateClient", "(Ljava/lang/String;)V");
    if (java->updateClient == 0) {
        LOGI("GetMethodID error - updateClient");
	status = 1;
        goto err1;
    }

    client_connect(&client);

err1:
    free(client.priv);
    (*env)->ReleaseStringUTFChars(env, string, path);

    return status;
#if 0
    jclass clazz = (*env)->FindClass(env, "com/getmyfiles/gui/GetMyFilesActivity");

    if (clazz == 0) {
        LOGI("FindClass error");
        return 1;
    }

    jmethodID javamethod = (*env)->GetMethodID(env, clazz, "showUrl", "()V");
    if (javamethod == 0) {
        LOGI("GetMethodID error");
        return 1;
    }

    (*env)->CallVoidMethod(env, this, javamethod);
#endif

#if 0
    (*env)->GetJavaVM(env, &javaVM);
    jclass cls = (*env)->GetObjectClass(env, this);
    activityClass = (jclass) (*env)->NewGlobalRef(env, cls);
    activityObj = (*env)->NewGlobalRef(env, this);

//    enver = env;
//    activityClass = (*env)->GetObjectClass(env, this);

    int stringCount = (*env)->GetArrayLength(env, stringArray);

    int i = 0;
    jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
    const char *path = (*env)->GetStringUTFChars(env, string, NULL);

    client.enable_httpd = 1;
    client.max_ext_conns = 10;
    client.max_int_conns = 10;
    client.host = "getmyfil.es";
    client.port = 8100;
    client.root_dir = path;
    client.exit_request = 0;
    client.id = 0;

    client_connect(&client);

    (*env)->ReleaseStringUTFChars(env, string, path);
#endif
}

JNIEXPORT void JNICALL Java_com_getmyfiles_gui_GetMyFilesActivity_disconnectClient
  (JNIEnv *env, jclass this)
{
    client.exit_request = 1;
}

void show_server_directory(client_args *client, char *str)
{
    JavaVars *java = (JavaVars *)client->priv;

    jstring jstr = (*java->env)->NewStringUTF(java->env, str);

    (*java->env)->CallVoidMethod(java->env, java->this, java->showServerDirectory, jstr);
}

void update_client(client_args *client, char *vers)
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
