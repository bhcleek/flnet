// generated by Fast Light User Interface Designer (fluid) version 1.0303

#ifndef netutils_h
#define netutils_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
extern Fl_Browser *brwsCallsign;
#include <FL/Fl_Button.H>
extern void cb_btnCancelCallsignSearch(Fl_Button*, void*);
extern Fl_Button *btnCancelCallsignSearch;
extern void cb_OkCallsignSearch(Fl_Button*, void*);
extern Fl_Button *btn_OkCallsignSearch;
Fl_Double_Window* winCallsignBrowse();
extern Fl_Browser *brwsNetNbr;
extern void cb_btnCancelNetNbrSearch(Fl_Button*, void*);
extern Fl_Button *btnCancelNetNbrSearch;
extern void cb_OkNetNbrSearch(Fl_Button*, void*);
extern Fl_Button *btn_OkNetNbrSearch;
Fl_Double_Window* winNetNbrBrowse();
extern Fl_Double_Window *winSearchCallsignDialog;
#include <FL/Fl_Input.H>
extern Fl_Input *sSrchPrefix;
extern Fl_Input *sSrchArea;
extern Fl_Input *sSrchSuffix;
extern void cb_btnSearchCancel(Fl_Button*, void*);
extern Fl_Button *btnSearchCancel;
#include <FL/Fl_Return_Button.H>
extern void cb_btnSearchOK(Fl_Return_Button*, void*);
extern Fl_Return_Button *btnSearchOK;
#include <FL/Fl_Box.H>
Fl_Double_Window* newSearchCallsignDialog();
extern Fl_Double_Window *winSearchNetNbrDialog;
extern Fl_Input *sSrchNetNbr;
extern void cb_btnSearchNetNbrCancel(Fl_Button*, void*);
extern Fl_Button *btnSearchNetNbrCancel;
extern void cb_btnSearchNetNbrOK(Fl_Return_Button*, void*);
extern Fl_Return_Button *btnSearchNetNbrOK;
Fl_Double_Window* newSearchNetNbrDialog();
extern Fl_Double_Window *dlgAbout;
#include <FL/Fl_Group.H>
extern Fl_Group *lblVersion;
Fl_Double_Window* newAboutDialog();
#endif
