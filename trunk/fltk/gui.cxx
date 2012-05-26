// generated by Fast Light User Interface Designer (fluid) version 1.0300

#include "gui.h"
#include "../utils.h"
static int fConnected = 0; 
Fl_Preferences prefs_(Fl_Preferences::USER, "fltk.org", "getmyfiles"); 

Fl_Button *closeButton=(Fl_Button *)0;

static void cb_closeButton(Fl_Button*, void*) {
  exit(0);
}

Fl_Output *urlText=(Fl_Output *)0;

Fl_Button *browseButton=(Fl_Button *)0;

static void cb_browseButton(Fl_Button*, void*) {
  Fl_Native_File_Chooser fc;
	fc.title("Open");
	fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	switch ( fc.show() ) {
	case -1: break;	// Error
	case  1: break; 	// Cancel
	default:		// Choice
		fc.preset_file(fc.filename());
#ifdef _WIN32
		{
			wchar_t szFile[_MAX_PATH];
			char szFile1[_MAX_PATH];
			ACPToUnicode16((CHAR *)fc.filename(), szFile, _MAX_PATH);
			Unicode16ToUtf8(szFile, szFile1, _MAX_PATH);
			folderText->value(szFile1);
		}
#else
		folderText->value(fc.filename());
#endif
		fprintf(stderr, "open %s\n", fc.filename());
		break;
	};
}

Fl_Output *folderText=(Fl_Output *)0;

Fl_Light_Button *connectButton=(Fl_Light_Button *)0;

static void cb_connectButton(Fl_Light_Button*, void*) {
  if (!fConnected) {
#ifdef _WIN32
	wchar_t szFile[_MAX_PATH];
	char szFile1[_MAX_PATH];
	Utf8ToUnicode16((CHAR *)folderText->value(), szFile, _MAX_PATH);
	Unicode16ToACP(szFile, szFile1, _MAX_PATH);
	fConnected = online_client(szFile1);
#else
	fConnected = online_client(folderText->value());
#endif
	if (fConnected) {
		browseButton->deactivate();
//		shareButton->value(1);
	}
} else {
	fConnected = offline_client();
	if (!fConnected) {
		browseButton->activate();
//		shareButton->value(0);
		urlText->value("");
	}
};
}

Fl_Box *infoStr=(Fl_Box *)0;

Fl_Button *confButton=(Fl_Button *)0;

static void cb_confButton(Fl_Button*, void*) {
  int val;
Fl_Double_Window *w = make_conf_window();
prefs_.get("httpd", val, 1);
enableHttpd->value(val);
prefs_.get("max_conn", val, 0);
maxClients->value(val);
w->show();
}

Fl_Double_Window* make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(535, 110, "GetMyFil.es");
    w = o;
    o->align(Fl_Align(FL_ALIGN_CLIP|FL_ALIGN_INSIDE));
    { closeButton = new Fl_Button(455, 80, 75, 25, "Quit");
      closeButton->callback((Fl_Callback*)cb_closeButton);
    } // Fl_Button* closeButton
    { urlText = new Fl_Output(55, 45, 395, 25, "URL");
    } // Fl_Output* urlText
    { browseButton = new Fl_Button(455, 10, 75, 25, "Browse");
      browseButton->callback((Fl_Callback*)cb_browseButton);
    } // Fl_Button* browseButton
    { folderText = new Fl_Output(55, 10, 395, 25, "Folder");
    } // Fl_Output* folderText
    { connectButton = new Fl_Light_Button(455, 45, 75, 25, "Share");
      connectButton->selection_color(FL_RED);
      connectButton->callback((Fl_Callback*)cb_connectButton);
    } // Fl_Light_Button* connectButton
    { infoStr = new Fl_Box(5, 80, 445, 25, "ItsMyFile client");
    } // Fl_Box* infoStr
    { confButton = new Fl_Button(5, 80, 25, 25);
      confButton->callback((Fl_Callback*)cb_confButton);
    } // Fl_Button* confButton
    o->size_range(535, 110, 535, 110);
    o->end();
  } // Fl_Double_Window* o
  return w;
}

void set_online() {
  fConnected = 1;
  browseButton->deactivate();
  //urlButton->value("Online");
  connectButton->value(1);
}

void set_offline() {
  fConnected = 0;
  browseButton->activate();
  //urlButton->value("Online");
  connectButton->value(0);
  urlText->value("");
}

void show_server_directory(char *dir) {
  urlText->value(dir);
}

void show_shared_directory(char *dir) {
  folderText->value(dir);
}

Fl_Spinner *maxClients=(Fl_Spinner *)0;

Fl_Check_Button *enableHttpd=(Fl_Check_Button *)0;

Fl_Button *closeConfButton=(Fl_Button *)0;

static void cb_closeConfButton(Fl_Button* o, void*) {
  prefs_.set("httpd", enableHttpd->value());
prefs_.set("max_conn", maxClients->value());
o->parent()->hide();
delete(o->parent());
}

Fl_Double_Window* make_conf_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(310, 95, "Settings");
    w = o;
    { maxClients = new Fl_Spinner(5, 5, 55, 25, "Maximum number of connections");
      maxClients->minimum(0);
      maxClients->maximum(999);
      maxClients->align(Fl_Align(FL_ALIGN_RIGHT));
    } // Fl_Spinner* maxClients
    { enableHttpd = new Fl_Check_Button(5, 35, 300, 25, "Enable direct connection to this client");
      enableHttpd->down_box(FL_DOWN_BOX);
    } // Fl_Check_Button* enableHttpd
    { closeConfButton = new Fl_Button(230, 65, 75, 25, "Close");
      closeConfButton->callback((Fl_Callback*)cb_closeConfButton);
    } // Fl_Button* closeConfButton
    o->set_modal();
    o->end();
  } // Fl_Double_Window* o
  return w;
}
