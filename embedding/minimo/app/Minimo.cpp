/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Christopher Blizzard. Portions created by Christopher Blizzard are Copyright (C) Christopher Blizzard.  All Rights Reserved.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Doug Turner <dougt@meer.net>  Branched from TestGtkEmbed.cpp
 *
 *   The 10LE Team (in alphabetical order)
 *   -------------------------------------
 *
 *    Carlos Aguiar     <carlos.aguiar@indt.org.br>
 *    Ilias Biris	<ext-ilias.biris@indt.org.br> - Coordinator
 *    Antonio Gomes     <agan@ufam.edu.br>
 *    Diego Gonzalez    <diego.gonzalez@indt.org.br>
 *    Joao Leite        <joao.leite@indt.org.br>
 *    Edjard Mota       <edjard.mota@indt.org.br> - Team Leader/Manager
 *    Andre Pedralho    <asp@ufam.edu.br>
 *    David Oliveira    <david@ufam.edu.br>
 *    Afonso Rabelo     <afonso.rabelo@indt.org.br>
 *    Claudia Roessing  <claudia.roessing@indt.org.br>  
 *
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "gtkmozembed.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

/* bookmark library */
#include "bookmark.h"

/* mozilla specific headers */
#include "prenv.h"
#include "nsMemory.h"
#include "gtkmozembed_internal.h"

/* Includes used by Find features */
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFind.h"
#include "nsNetUtil.h"

/* Includes used by 'mozilla_preference_set', 'mozilla_preference_set_int' and 'mozilla_preference_set_boolean' */
#include "nsIPref.h"

/* Includes used by 'save_mozilla' */
#include "nsIPresShell.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeItem.h"
#include "nsILocalFile.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocument.h"

#define PREF_ID NS_PREF_CONTRACTID

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#ifdef MOZ_JPROF
#include "jprof.h"
#endif


/* MINIMO TOOLBAR STRUCT */
typedef struct _MinimoToolBar {
  GtkWidget  *toolbar;
  
  GtkWidget *OpenButton;
  GtkWidget *BackButton;
  GtkWidget *ReloadButton;
  GtkWidget *ForwardButton;
  GtkWidget *StopButton;
  GtkWidget *PrefsButton;
  GtkWidget *InfoButton;
  GtkWidget *QuitButton;
  
} MinimoToolbar;

/* MINIMO STRUCT */
typedef struct _MinimoBrowser {
  
  GtkWidget  *topLevelWindow;
  GtkWidget  *topLevelVBox;
  GtkWidget  *mozEmbed;
  
  MinimoToolbar toolbar;
  
  gboolean    show_tabs;
  GtkWidget  *notebook;
  GtkWidget  *label_notebook;
  GtkWidget  *active_page;
  GtkWidget  *expanderBar;
  GtkWidget  *ToolbarDown;
  GtkWidget  *TabButton;
  GtkWidget  *AddTabButton;
  GtkWidget  *CloseTabButton;
  GtkWidget  *OpenBookmarkButton;
  GtkWidget  *AddBookmarkButton;
  GtkWidget  *OpenFindDialog;
  GtkWidget  *OpenSaveAsBuntton;
  GtkWidget  *OpenFromFileButton;
  
  GtkWidget  *progressPopup;
  GtkWidget  *progressBar;
  gint        totalBytes;
  
  gboolean didFind;
  
} MinimoBrowser;

/* Global Minimo Variable */
MinimoBrowser *browser;

/* SPECIFIC STRUCT FOR SPECIFY SOME PARAMETERS OF '_open_dialog_params' METHOD */
typedef struct _open_dialog_params
{
  GtkWidget* dialog_combo;
  MinimoBrowser* browser;
} OpenDialogParams;

/* Minimo's PATH - probably "~/home/current_user/.Minimo" */
static char *g_profile_path = NULL;

/* the list of browser windows currently open */
GList *browser_list = g_list_alloc();

/* store the last expression on find dialog */
gchar *last_find = NULL;

/* Number of browsers */
static int num_browsers = 0;

/* Last clicked URL */
gchar *clicked_url;

/* History */
GList* g_autocomplete_list = g_list_alloc();

/* Data Struct for make possible the real autocomplete */
GtkEntryCompletion *minimo_entry_completion;
GtkListStore *store;
GtkTreeIter iter;

/* METHODS HEADERS  */

/*callbacks from the expander */
static void open_new_tab_cb (GtkButton *button, MinimoBrowser *browser);
static void close_current_tab_cb (GtkMenuItem *menuitem, MinimoBrowser *browser);
static void show_hide_tabs_cb (GtkButton *button, MinimoBrowser *browser);
static void open_bookmark_window_cb (GtkButton *menuitem, MinimoBrowser *browser);
static void switch_page_cb (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, MinimoBrowser *browser);
static GtkWidget *get_active_tab (MinimoBrowser *browser);
static void link_message_cb (GtkWidget *, MinimoBrowser *);
static void update_ui (MinimoBrowser *browser);
static void open_bookmark_window_cb (GtkButton *button,MinimoBrowser *browser);
static void create_find_dialog (GtkWidget *w, MinimoBrowser *browser);
static void create_save_document(GtkButton *button, MinimoBrowser *browser);
void create_open_document_from_file (GtkButton *button, MinimoBrowser *browser);

/* Building the Minimo Browser */
static MinimoBrowser *new_gtk_browser(guint32 chromeMask);
static GtkWidget* create_minimo_progress(MinimoBrowser* browser);
static void create_minimo_expander_bar(MinimoBrowser* browser);
static void set_browser_visibility(MinimoBrowser *browser, gboolean visibility);
static void setup_toolbar(MinimoBrowser* browser);
static void quick_message(gchar* message);
static void build_entry_completion (MinimoBrowser* browser);
static void setup_escape_key_handler(GtkWidget *window);
static gint escape_key_handler(GtkWidget *window, GdkEventKey *ev);

/* callbacks from the UI */
static gboolean delete_cb(GtkWidget *widget, GdkEventAny *event, MinimoBrowser *browser);
static void destroy_cb(GtkWidget *widget, MinimoBrowser *browser);

/* callbacks from the widget */
static void title_changed_cb(GtkMozEmbed *embed, MinimoBrowser *browser);
static void load_started_cb(GtkMozEmbed *embed, MinimoBrowser *browser);
static void load_finished_cb(GtkMozEmbed *embed, MinimoBrowser *browser);
static void new_window_cb(GtkMozEmbed *embed, GtkMozEmbed **retval, guint chromemask, MinimoBrowser *browser);
static void visibility_cb(GtkMozEmbed *embed, gboolean visibility, MinimoBrowser *browser);
//static void destroy_brsr_cb(GtkMozEmbed *embed, MinimoBrowser *browser);
static gint open_uri_cb(GtkMozEmbed *embed, const char *uri, MinimoBrowser *browser);
static void size_to_cb(GtkMozEmbed *embed, gint width, gint height, MinimoBrowser *browser);
static void create_info_dialog (MinimoBrowser* browser);
static void create_open_dialog(MinimoBrowser* browser);
static void find_dialog_ok_cb (GtkWidget *w, GtkWidget *window);
static void on_save_ok_cb(GtkWidget *button, GtkWidget *fs);
static void on_open_ok_cb(GtkWidget *button, GtkWidget *fs);

/* callbacks from the singleton object */
static void new_window_orphan_cb(GtkMozEmbedSingle *embed, GtkMozEmbed **retval, guint chromemask, gpointer data);

/* Mozilla API Methods - make possible the direct comunitation between Minimo Browser and the "Gecko Render Engine" */

static PRUnichar *mozilla_locale_to_unicode (const gchar *locStr);
static gboolean mozilla_find(GtkMozEmbed *b, const char *exp, PRBool IgnoreCase,
                             PRBool SearchBackWards, PRBool DoWrapFind,
                             PRBool SearchEntireWord, PRBool SearchInFrames,
                             PRBool DidFind);
static gboolean mozilla_preference_set (const char *preference_name, const char *new_value);
static gboolean mozilla_preference_set_boolean (const char *preference_name, gboolean new_boolean_value);
static gboolean mozilla_preference_set_int (const char *preference_name, gint  new_int_value);
static gboolean mozilla_save(GtkMozEmbed *b, gchar *file_name, gint all);
static gchar *mozilla_get_link_message (GtkWidget *embed);

/*
 * mozilla_locale_to_unicode: Decodes a string encoded for the current
 * locale into unicode. Used for getting text entered in a GTK+ entry
 * into a form which mozilla can use.
 *
 */
static PRUnichar *mozilla_locale_to_unicode (const gchar *locStr)
{
  
  if (locStr == NULL)
  {
    return (NULL);
  }
  nsAutoString autoStr;
  autoStr.AssignWithConversion (locStr);       
  PRUnichar *uniStr = ToNewUnicode(autoStr);
  return ( uniStr );
}

/*
 * mozilla_find: Look for a text in the current shown webpage
 *
 */
static gboolean 
mozilla_find(GtkMozEmbed *b, const char *exp, PRBool IgnoreCase,
             PRBool SearchBackWards, PRBool DoWrapFind,
             PRBool SearchEntireWord, PRBool SearchInFrames,
             PRBool DidFind)
{
  PRUnichar *search_string;
  g_return_val_if_fail(b != NULL, FALSE);
  nsIWebBrowser *wb = nsnull;
  gtk_moz_embed_get_nsIWebBrowser( b, &wb);
  nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(wb));
  search_string = mozilla_locale_to_unicode(exp);
  finder->SetSearchString(search_string);
  
  finder->SetFindBackwards(SearchBackWards);
  finder->SetWrapFind(DoWrapFind);
  finder->SetEntireWord(SearchEntireWord);
  finder->SetSearchFrames(SearchInFrames);
  finder->SetMatchCase(IgnoreCase);
  finder->FindNext(&DidFind);
  
  g_free(search_string);
  return ( DidFind );
}

/*
 * mozilla_preference_set: Method used to set up some specific text-based parameters of the gecko render engine
 *
 */
static gboolean
mozilla_preference_set (const char *preference_name, const char *new_value)
{
  g_return_val_if_fail (preference_name != NULL, FALSE);
  g_return_val_if_fail (new_value != NULL, FALSE);
  
  nsCOMPtr<nsIPref> pref = do_CreateInstance(PREF_ID);
  
  if (pref)
  {
    nsresult rv = pref->SetCharPref (preference_name, new_value);
    return ( NS_SUCCEEDED (rv) ? TRUE : FALSE );
  }
  return (FALSE);
}

/*
 * mozilla_preference_set: Method used to set up some specific boolean-based parameters of the gecko render engine
 *
 */
static gboolean
mozilla_preference_set_boolean (const char *preference_name, gboolean new_boolean_value)
{
  
  g_return_val_if_fail (preference_name != NULL, FALSE);
  
  nsCOMPtr<nsIPref> pref = do_CreateInstance(PREF_ID);
  
  if (pref)
  {
    nsresult rv = pref->SetBoolPref (preference_name,  new_boolean_value ? PR_TRUE : PR_FALSE);
    return ( NS_SUCCEEDED (rv) ? TRUE : FALSE );
  }
  
  return (FALSE);
}

/*
 * mozilla_preference_set: Method used to set up some specific integer-based parameters of the gecko render engine
 *
 */
static gboolean
mozilla_preference_set_int (const char *preference_name, gint  new_int_value)
{
  g_return_val_if_fail (preference_name != NULL, FALSE);
  
  nsCOMPtr<nsIPref> pref = do_CreateInstance(PREF_ID);
  
  if (pref)
  {
    nsresult rv = pref->SetIntPref (preference_name, new_int_value);
    return ( NS_SUCCEEDED (rv) ? TRUE : FALSE );
  }
  
  return (FALSE);
  
}

/*
 * mozilla_save: Method used to save an webpage and its files into the disk
 *
 */
static gboolean
mozilla_save(GtkMozEmbed *b, gchar *file_name, gint all)
{
  
  nsCOMPtr<nsIWebBrowser> wb;
  gint i = 0;
  gchar *relative_path = NULL;
  g_return_val_if_fail(b != NULL, FALSE);
  
  gtk_moz_embed_get_nsIWebBrowser(b,getter_AddRefs(wb));
  if (!wb) return (FALSE);
  
  nsCOMPtr<nsIWebNavigation> nav(do_QueryInterface(wb));
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  nav->GetDocument(getter_AddRefs(domDoc));
  
  if (!domDoc) return (FALSE);
  nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(wb));
  
  if (persist) {
    nsCOMPtr<nsILocalFile> file;
    nsCOMPtr<nsILocalFile> relative = nsnull;
    if (all) {
      
      relative_path = g_strdup(file_name);
      relative_path = g_strconcat(relative_path,"_files/", NULL);
      
      for (i=strlen(relative_path)-1 ; i >= 0; --i)
        if (relative_path[i] == '/') {
          relative_path[i] = '\0';
          break;
        }
      
      mkdir(relative_path,0755);
      
      nsAutoString s; s.AssignWithConversion(relative_path);
      NS_NewLocalFile(s, PR_TRUE, getter_AddRefs(relative));
    }
    
    nsAutoString s; s.AssignWithConversion(file_name);
    NS_NewLocalFile(s,PR_TRUE,getter_AddRefs(file));
    
    if (file) persist->SaveDocument(domDoc,file,relative, nsnull, 0, 0);
    
    if (all) g_free(relative_path);
    
    return (TRUE);
  }
  return (FALSE);
}

/*
 * mozilla_get_link_message: Method used to get "link messages" from the Gecko Render Engine. 
 * eg. When the user put the mouse over a link, the render engine throws messegas that can be cautch for after 
 * used by Minimo Browser.
 *
 */
gchar *mozilla_get_link_message (GtkWidget *embed) {
  
  gchar *link;
  
  link = gtk_moz_embed_get_link_message (GTK_MOZ_EMBED (embed));
  
  return link;
  
}

static void handle_remote(int sig)
{
  FILE *fp;
  char url[256];
  
  sprintf(url, "/tmp/Mosaic.%d", getpid());
  if ((fp = fopen(url, "r"))) 
  {
    if (fgets(url, sizeof(url) - 1, fp))
    {
      MinimoBrowser *bw = NULL;
      if (strncmp(url, "goto", 4) == 0 && fgets(url, sizeof(url) - 1, fp)) 
      {
        GList *tmp_list = browser_list;
        bw =(MinimoBrowser *)tmp_list->data;
        if(!bw) return;
      }
      else if (strncmp(url, "newwin", 6) == 0 && fgets(url, sizeof(url) - 1, fp))
      {
        bw = new_gtk_browser(GTK_MOZ_EMBED_FLAG_DEFAULTCHROME);
        gtk_widget_set_usize(bw->mozEmbed, 240, 320);
        set_browser_visibility(bw, TRUE);
      }
      if (bw)
        gtk_moz_embed_load_url(GTK_MOZ_EMBED(bw->mozEmbed), url);
      fclose(fp);
    }
  }
  return;
}

static void init_remote()
{
  gchar *file;
  FILE *fp;
  
  signal(SIGUSR1, SIG_IGN);
  
  /* Write the pidfile : would be useful for automation process*/
  file = g_strconcat(g_get_home_dir(), "/", ".mosaicpid", NULL);
  if((fp = fopen(file, "w"))) 
  {
    fprintf (fp, "%d\n", getpid());
    fclose (fp);
    signal(SIGUSR1, handle_remote);
  }
  g_free(file);
}

static void cleanup_remote()
{
  gchar *file;
  
  signal(SIGUSR1, SIG_IGN);
  
  file = g_strconcat(g_get_home_dir(), "/", ".mosaicpid", NULL);
  unlink(file);
  g_free(file);
}

static void autocomplete_populate(gpointer data, gpointer combobox)
{
  if (!data || !combobox)
    return;
  
  gtk_combo_set_popdown_strings (GTK_COMBO (combobox), g_autocomplete_list);
}

static void populate_autocomplete(GtkCombo *comboBox)
{
  g_list_foreach(g_autocomplete_list, autocomplete_populate, comboBox);
}

static gint autocomplete_list_cmp(gconstpointer a, gconstpointer b)
{
  if (!a)
    return -1;
  if (!b)
    return 1;
  
  return strcmp((char*)a, (char*)b);
}

static void init_autocomplete()
{
  if (!g_autocomplete_list)
    return;
  
  char* full_path = g_strdup_printf("%s/%s", g_profile_path, "autocomplete.txt");
  
  FILE *fp;
  char url[255];
  
  store = gtk_list_store_new (1, G_TYPE_STRING);
  
  if((fp = fopen(full_path, "r+"))) 
  {
    while(fgets(url, sizeof(url) - 1, fp))
    {
      int length = strlen(url);
      if (url[length-1] == '\n')
        url[length-1] = '\0'; 
      g_autocomplete_list = g_list_append(g_autocomplete_list, g_strdup(url));
      /* Append url's to autocompletion feature */
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, url, -1);
    }
    fclose(fp);
  }
  
  g_autocomplete_list =   g_list_sort(g_autocomplete_list, autocomplete_list_cmp);
}

static void autocomplete_writer(gpointer data, gpointer fp)
{
  FILE* file = (FILE*) fp;
  char* url  = (char*) data;
  
  if (!url || !fp)
    return;
  
  fwrite(url, strlen(url), 1, file);
  fputc('\n', file);
  
}

static void autocomplete_destroy(gpointer data, gpointer dummy)
{
  g_free(data);
}

static void cleanup_autocomplete()
{
  char* full_path = g_strdup_printf("%s/%s", g_profile_path, "autocomplete.txt");
  
  FILE *fp;
  if((fp = fopen(full_path, "w"))) 
  {
    g_list_foreach(g_autocomplete_list, autocomplete_writer, fp);
    fclose(fp);
  }
  
  g_free(full_path);
  g_list_foreach(g_autocomplete_list, autocomplete_destroy, NULL);
}

static void add_autocomplete(const char* value, OpenDialogParams* params)
{
  GList* found = g_list_find_custom(g_autocomplete_list, value, autocomplete_list_cmp);
  if (!found) {
    
    g_autocomplete_list = g_list_insert_sorted(g_autocomplete_list, g_strdup(value), autocomplete_list_cmp);    
    /* Append url's to autocompletion*/
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, value, -1);
  }
  gtk_combo_set_popdown_strings (GTK_COMBO (params->dialog_combo), g_autocomplete_list);
}

int
main(int argc, char **argv) {
  
#ifdef NS_TRACE_MALLOC
  argc = NS_TraceMallocStartupArgs(argc, argv);
#endif
  
  gtk_set_locale();
  gtk_init(&argc, &argv);
  
  
  char *home_path;
  home_path = PR_GetEnv("HOME");
  if (!home_path) {
    fprintf(stderr, "Failed to get HOME\n");
    exit(1);
  }
  
  g_profile_path = g_strdup_printf("%s/%s", home_path, ".Minimo");
  
  gtk_moz_embed_set_profile_path(g_profile_path, "Minimo");
  
  browser = new_gtk_browser(GTK_MOZ_EMBED_FLAG_DEFAULTCHROME);
  
  // set our minimum size
  gtk_widget_set_usize(browser->mozEmbed, 240, 320);
  
  set_browser_visibility(browser, TRUE);
  
  init_remote();
  init_autocomplete();
  
  if (argc > 1)
    gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser->mozEmbed), argv[1]);
  
#if 0
  /* testing popups settings */
  mozilla_preference_set_boolean ("dom.disable_open_during_load", 0);
  
  /* testing proxy settings */
  mozilla_preference_set_int    ("network.proxy.type", 1);
  mozilla_preference_set	("network.proxy.http", "172.18.110.24");
  mozilla_preference_set_int    ("network.proxy.http_port", 8080);
  mozilla_preference_set 	("network.proxy.ftp", "172.18.110.24");
  mozilla_preference_set_int    ("network.proxy.ftp_port", 8080);
  mozilla_preference_set 	("network.proxy.ssl", "172.18.110.24");
  mozilla_preference_set_int    ("network.proxy.ssl_port", 8080);
  mozilla_preference_set_boolean("nglayout.widget.gfxscrollbars", TRUE);
#endif
  
  // get the singleton object and hook up to its new window callback
  // so we can create orphaned windows.
  GtkMozEmbedSingle *single;
  
  single = gtk_moz_embed_single_get();
  if (!single) {
    fprintf(stderr, "Failed to get singleton embed object!\n");
    exit(1);
  }
  
  gtk_signal_connect(GTK_OBJECT(single), "new_window_orphan", GTK_SIGNAL_FUNC(new_window_orphan_cb), NULL);
  
  gtk_main();
  
  cleanup_remote();
  cleanup_autocomplete();
}

/* Build the Minimo Window */
static MinimoBrowser *
new_gtk_browser(guint32 chromeMask)
{
  MinimoBrowser *browser = 0;
  
  num_browsers++;
  
  browser = g_new0(MinimoBrowser, 1);
  
  browser_list = g_list_prepend(browser_list, browser);
  
  // create our new toplevel window
  browser->topLevelWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable (GTK_WINDOW(browser->topLevelWindow),TRUE);
  gtk_window_set_title (GTK_WINDOW (browser->topLevelWindow), "MINIMO - Embedded Mozilla");
  
  // new vbox
  browser->topLevelVBox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show (browser->topLevelVBox);
  
  // add it to the toplevel window
  gtk_container_add(GTK_CONTAINER(browser->topLevelWindow), browser->topLevelVBox);
  
  setup_toolbar(browser);
  
  gtk_box_pack_start(GTK_BOX(browser->topLevelVBox), 
                     browser->toolbar.toolbar,
                     FALSE, // expand
                     FALSE, // fill
                     0);    // padding
  
  browser->notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (browser->topLevelVBox), browser->notebook, TRUE, TRUE, 0);
  browser->show_tabs = FALSE;
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK(browser->notebook), browser->show_tabs);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (browser->notebook), GTK_POS_LEFT);
  gtk_widget_show (browser->notebook);
  
  open_new_tab_cb (NULL, browser);
  
  // catch the destruction of the toplevel window
  gtk_signal_connect(GTK_OBJECT(browser->topLevelWindow), "delete_event", 
                     GTK_SIGNAL_FUNC(delete_cb), browser);
  
  // hookup to when the window is destroyed
  gtk_signal_connect(GTK_OBJECT(browser->topLevelWindow), "destroy", GTK_SIGNAL_FUNC(destroy_cb), browser);
  
  gtk_signal_connect (GTK_OBJECT (browser->notebook), "switch_page", 
                      GTK_SIGNAL_FUNC (switch_page_cb), browser);
  
  
  // set the chrome type so it's stored in the object
  gtk_moz_embed_set_chrome_mask(GTK_MOZ_EMBED(browser->mozEmbed), chromeMask);
  
  create_minimo_expander_bar(browser);
  
  browser->progressPopup = create_minimo_progress(browser);
  
  // add it to the toplevel vbox
  gtk_box_pack_start(GTK_BOX(browser->topLevelVBox), 
                     browser->progressPopup,
                     FALSE, // expand
                     FALSE, // fill
                     0);   // padding
  
  browser->active_page = get_active_tab (browser);
  
  return browser;
}

void
set_browser_visibility(MinimoBrowser *browser, gboolean visibility)
{
  if (!visibility)
  {
    gtk_widget_hide(browser->topLevelWindow);
    return;
  }
  
  gtk_widget_show(browser->mozEmbed);
  gtk_widget_show(browser->topLevelVBox);
  gtk_widget_show(browser->topLevelWindow);
}

/* bookmark button clicked */ 
void open_bookmark_window_cb (GtkButton *button,
                              MinimoBrowser *browser)
{
  show_bookmark(browser->mozEmbed);
}

void
back_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  gtk_moz_embed_go_back(GTK_MOZ_EMBED(browser->mozEmbed));
}

void reload_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  GdkModifierType state = (GdkModifierType)0;
  gint x, y;
  gdk_window_get_pointer(NULL, &x, &y, &state);
  
  gtk_moz_embed_reload(GTK_MOZ_EMBED(browser->mozEmbed),
                       (state & GDK_SHIFT_MASK) ?
                       GTK_MOZ_EMBED_FLAG_RELOADBYPASSCACHE : 
                       GTK_MOZ_EMBED_FLAG_RELOADNORMAL);
}

void
forward_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  gtk_moz_embed_go_forward(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
info_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  create_info_dialog(browser);
}

void
pref_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
}

void
stop_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  gtk_moz_embed_stop_load(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
open_clicked_cb(GtkButton *button, MinimoBrowser *browser)
{
  create_open_dialog(browser);
}

gboolean
delete_cb(GtkWidget *widget, GdkEventAny *event, MinimoBrowser *browser)
{
  gtk_widget_destroy(widget);
  return TRUE;
}

void
destroy_cb(GtkWidget *widget, MinimoBrowser *browser)
{
  GList *tmp_list;
  num_browsers--;
  tmp_list = g_list_find(browser_list, browser);
  browser_list = g_list_remove_link(browser_list, tmp_list);
  if (num_browsers == 0)
    gtk_main_quit();
  
}

void
title_changed_cb(GtkMozEmbed *embed, MinimoBrowser *browser)
{
  char *newTitle;
  newTitle = gtk_moz_embed_get_title(embed);
  if(newTitle)
  {
    gtk_window_set_title(GTK_WINDOW(browser->topLevelWindow), newTitle);
    g_free(newTitle);
  }
}

void
load_started_cb(GtkMozEmbed *embed, MinimoBrowser *browser)
{
  browser->totalBytes = 0;
  gtk_widget_show_all(browser->progressPopup);
}

void
load_finished_cb(GtkMozEmbed *embed, MinimoBrowser *browser)
{
  gtk_widget_hide_all(browser->progressPopup);
  nsMemory::HeapMinimize(PR_TRUE);
}

void
new_window_cb(GtkMozEmbed *embed, GtkMozEmbed **newEmbed, guint chromemask, MinimoBrowser *browser)
{
  
  open_new_tab_cb (NULL, browser);
  gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser->mozEmbed), clicked_url);
  show_hide_tabs_cb (NULL, browser);
}

void
visibility_cb(GtkMozEmbed *embed, gboolean visibility, MinimoBrowser *browser)
{
  set_browser_visibility(browser, visibility);
}

gint
open_uri_cb(GtkMozEmbed *embed, const char *uri, MinimoBrowser *browser)
{
  // don't interrupt anything
  return FALSE;
}

void
size_to_cb(GtkMozEmbed *embed, gint width, gint height,
           MinimoBrowser *browser)
{
  gtk_widget_set_usize(browser->mozEmbed, width, height);
}

void
link_message_cb (GtkWidget *embed, MinimoBrowser *browser)
{
  
  clicked_url = mozilla_get_link_message (browser->active_page);
  
}

void new_window_orphan_cb(GtkMozEmbedSingle *embed,
                          GtkMozEmbed **retval, guint chromemask,
                          gpointer data)
{
  MinimoBrowser *newBrowser = new_gtk_browser(chromemask);
  *retval = GTK_MOZ_EMBED(newBrowser->mozEmbed);
}

static PangoFontDescription* gDefaultMinimoFont = NULL;
static PangoFontDescription* getOrCreateDefaultMinimoFont()
{
  if (gDefaultMinimoFont)
    return gDefaultMinimoFont;
  
  gDefaultMinimoFont = pango_font_description_from_string("sans 8");
  return gDefaultMinimoFont;
}

static void quick_message(gchar* message)
{
  GtkWidget *dialog, *label, *okbutton;
  
  dialog = gtk_dialog_new();
  label  = gtk_label_new(message);
  okbutton = gtk_button_new_with_label("Okay");
  
  gtk_widget_modify_font(label, getOrCreateDefaultMinimoFont());
  gtk_widget_modify_font(okbutton, getOrCreateDefaultMinimoFont());
  
  gtk_signal_connect_object(GTK_OBJECT(okbutton), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), okbutton);
  
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
  gtk_widget_show_all(dialog);
}

void setup_toolbar(MinimoBrowser* browser)
{
  MinimoToolbar* toolbar = &browser->toolbar;
  
#ifdef MOZ_WIDGET_GTK
  toolbar->toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
#endif
  
#ifdef MOZ_WIDGET_GTK2
  toolbar->toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar->toolbar),GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar->toolbar),GTK_TOOLBAR_ICONS);
#endif /* MOZ_WIDGET_GTK2 */
  
  gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar->toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
  
  gtk_widget_show(toolbar->toolbar);
  
  toolbar->OpenButton =(GtkWidget*) gtk_tool_button_new_from_stock ("gtk-open");
  gtk_widget_show(toolbar->OpenButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->OpenButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->OpenButton), "clicked", GTK_SIGNAL_FUNC(open_clicked_cb), browser);
  
  toolbar->BackButton = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-go-back");
  gtk_widget_show(toolbar->BackButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->BackButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->BackButton), "clicked", GTK_SIGNAL_FUNC(back_clicked_cb), browser);
  
  toolbar->ReloadButton = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-refresh");
  gtk_widget_show(toolbar->ReloadButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->ReloadButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->ReloadButton), "clicked", GTK_SIGNAL_FUNC(reload_clicked_cb), browser);
  
  toolbar->ForwardButton = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-go-forward");
  gtk_widget_show(toolbar->ForwardButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->ForwardButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->ForwardButton), "clicked", GTK_SIGNAL_FUNC(forward_clicked_cb), browser);
  
  toolbar->StopButton =(GtkWidget*) gtk_tool_button_new_from_stock ("gtk-stop");
  gtk_widget_show (toolbar->StopButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->StopButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->StopButton), "clicked", GTK_SIGNAL_FUNC(stop_clicked_cb), browser);
  
  toolbar->PrefsButton =(GtkWidget*) gtk_tool_button_new_from_stock ("gtk-preferences");
  gtk_widget_show (toolbar->PrefsButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->PrefsButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->PrefsButton), "clicked", GTK_SIGNAL_FUNC(pref_clicked_cb), browser);
  
  toolbar->InfoButton = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-dialog-info");
  gtk_widget_show(toolbar->InfoButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->InfoButton);
  gtk_signal_connect(GTK_OBJECT(toolbar->InfoButton), "clicked", GTK_SIGNAL_FUNC(info_clicked_cb), browser);
  
  toolbar->QuitButton = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-quit");
  gtk_widget_show(toolbar->QuitButton);
  gtk_container_add(GTK_CONTAINER (toolbar->toolbar), toolbar->QuitButton);  
  gtk_signal_connect(GTK_OBJECT(toolbar->QuitButton), "clicked", GTK_SIGNAL_FUNC(destroy_cb), browser);
}

static void destroy_dialog_params_cb(GtkWidget *widget, OpenDialogParams* params)
{
  free(params);
}

static void open_dialog_ok_cb(GtkButton *button, OpenDialogParams* params)
{
  const gchar *url = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO(params->dialog_combo)->entry), 0, -1);
  
  gtk_moz_embed_load_url(GTK_MOZ_EMBED(params->browser->mozEmbed), url);
  
  add_autocomplete(url, params);
}

void
net_state_change_cb(GtkMozEmbed *embed, gint flags, guint status, GtkLabel* label)
{
  char * message = "Waiting...";
  
  if(flags & GTK_MOZ_EMBED_FLAG_IS_REQUEST) 
  {
    if (flags & GTK_MOZ_EMBED_FLAG_REDIRECTING)
      message = "Redirecting to site...";
    else if (flags & GTK_MOZ_EMBED_FLAG_TRANSFERRING)
      message = "Transferring data from site...";
    else if (flags & GTK_MOZ_EMBED_FLAG_NEGOTIATING)
      message = "Waiting for authorization...";
  }
  
  if (status == GTK_MOZ_EMBED_STATUS_FAILED_DNS)
    message = "Site not found.";
  else if (status == GTK_MOZ_EMBED_STATUS_FAILED_CONNECT)
    message = "Failed to connect to site.";
  else if (status == GTK_MOZ_EMBED_STATUS_FAILED_TIMEOUT)
    message = "Failed due to connection timeout.";
  else if (status == GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED)
    message = "User canceled connecting to site.";
  
  if (flags & GTK_MOZ_EMBED_FLAG_IS_DOCUMENT) {
    if (flags & GTK_MOZ_EMBED_FLAG_START)
      message = "Loading site...";
    else if (flags & GTK_MOZ_EMBED_FLAG_STOP)
      message = "Done.";
  }
  
  gtk_label_set_text(label, message);
}

void
location_changed_cb(GtkMozEmbed *embed, GtkLabel* label)
{
  char *newLocation;
  newLocation = gtk_moz_embed_get_location(embed);
  if (newLocation)
  {
    gtk_label_set_text(label, newLocation);
    g_free(newLocation);
  }
}

void progress_change_cb(GtkMozEmbed *embed, gint cur, gint max, MinimoBrowser* browser)
{
  browser->totalBytes += cur;
  
  if (max < 1)
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(browser->progressBar));
  else
    gtk_progress_bar_update(GTK_PROGRESS_BAR(browser->progressBar), cur/max);
}

static GtkWidget* create_minimo_progress (MinimoBrowser* browser)
{
  
  if (!browser) {
    quick_message("Creating the progress meter failed.");
    return NULL;
  }
  
  GtkWidget *Minimo_Progress;
  GtkWidget *dialog_vbox4;
  GtkWidget *vbox3;
  GtkWidget *hbox2;
  GtkWidget *LoadingLabel;
  GtkWidget *URLLable;
  GtkWidget *progressbar1;
  GtkWidget *Minimo_Progress_Status;
  
  Minimo_Progress = gtk_vbox_new(FALSE, 0);
  
  dialog_vbox4 = Minimo_Progress;
  
  vbox3 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox3);
  gtk_box_pack_start(GTK_BOX (dialog_vbox4), vbox3, FALSE, FALSE, 0);
  
  hbox2 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX (vbox3), hbox2, FALSE, TRUE, 0);
  
  LoadingLabel = gtk_label_new("Loading:");
  gtk_widget_modify_font(LoadingLabel, getOrCreateDefaultMinimoFont());
  gtk_widget_show(LoadingLabel);
  gtk_box_pack_start(GTK_BOX (hbox2), LoadingLabel, FALSE, FALSE, 0);
  
  URLLable = gtk_label_new("...");
  gtk_widget_modify_font(URLLable, getOrCreateDefaultMinimoFont());
  gtk_widget_show(URLLable);
  gtk_box_pack_start(GTK_BOX (hbox2), URLLable, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL (URLLable), GTK_JUSTIFY_CENTER);
  
  progressbar1 = gtk_progress_bar_new ();
  gtk_widget_show(progressbar1);
  gtk_box_pack_start(GTK_BOX (vbox3), progressbar1, FALSE, FALSE, 0);
  gtk_widget_set_usize(progressbar1, 240, 20);
  
  Minimo_Progress_Status = gtk_label_new ("");
  gtk_widget_modify_font(Minimo_Progress_Status, getOrCreateDefaultMinimoFont());
  gtk_widget_show(Minimo_Progress_Status);
  gtk_box_pack_start(GTK_BOX (vbox3), Minimo_Progress_Status, TRUE, FALSE, 0);
  
  
  browser->progressBar = progressbar1;
  
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "net_state", GTK_SIGNAL_FUNC(net_state_change_cb), Minimo_Progress_Status);
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "progress",  GTK_SIGNAL_FUNC(progress_change_cb), browser);
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "location",  GTK_SIGNAL_FUNC(location_changed_cb), URLLable);
  
  return Minimo_Progress;
}

/* Create the 'Expand Bar', with buttons as "Hide/Show Tabs", "Add/Manage Bookmark", "Find", "Save As", "Open" ... */
static void create_minimo_expander_bar(MinimoBrowser* browser)
{
  
  if (!browser) {
    quick_message("Creating the expander bar failed.");
    return ;
  }
  
  browser->expanderBar = gtk_expander_new (NULL);
  gtk_widget_show (browser->expanderBar);
  gtk_box_pack_start (GTK_BOX (browser->topLevelVBox), browser->expanderBar, FALSE, TRUE, 0);
  
  browser->ToolbarDown = gtk_toolbar_new ();
  gtk_widget_show (browser->ToolbarDown);
  gtk_container_add (GTK_CONTAINER (browser->expanderBar), browser->ToolbarDown);
  gtk_widget_set_size_request (browser->ToolbarDown, -1, 20);
  gtk_toolbar_set_style (GTK_TOOLBAR (browser->ToolbarDown), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (browser->ToolbarDown), FALSE);
  
  /*
   * Show or hide notebooks
   */
  
  browser->TabButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Tab Button",
                            "Show/Hide Tabs",
                            "Show/Hide Tabs",
                            browser->TabButton, 
                            GTK_SIGNAL_FUNC(show_hide_tabs_cb),
                            browser);
  
  gtk_widget_set_size_request (browser->TabButton,16, 16);
  
  /*
   * Add new notebook
   */
  
  browser->AddTabButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Add Tab",
                            "Add Tab",
                            "Add Tab",
                            browser->AddTabButton, 
                            GTK_SIGNAL_FUNC(open_new_tab_cb),
                            browser);
  
  gtk_widget_set_size_request (browser->AddTabButton, 16, 16);
  
  /*
   * Close active notebook
   */
  
  browser->CloseTabButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Close Tab",
                            "Close Tab",
                            "Close Tab",
                            browser->CloseTabButton, 
                            GTK_SIGNAL_FUNC(close_current_tab_cb),
                            browser);
  
  gtk_widget_set_size_request (browser->CloseTabButton, 16, 16);
  
  /*
   * Open bookmark management window
   */
  
  browser->OpenBookmarkButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Open Bookmark Window",
                            "Open Bookmark Window",
                            "Open Bookmark Window",
                            browser->OpenBookmarkButton, 
                            GTK_SIGNAL_FUNC(open_bookmark_window_cb),
                            browser);
  
  gtk_widget_set_size_request (browser->CloseTabButton, 16, 16);
  
  /*
   * Add bookmark
   */
  
  browser->AddBookmarkButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Add Bookmark",
                            "Add Bookmark",
                            "Add Bookmark",
                            browser->AddBookmarkButton, 
                            GTK_SIGNAL_FUNC(add_bookmark_cb),
                            browser->mozEmbed);
  
  gtk_widget_set_size_request (browser->AddBookmarkButton, 16, 16);
  
  browser->OpenFindDialog =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Find",
                            "Find",
                            "Find",
                            browser->OpenFindDialog, 
                            GTK_SIGNAL_FUNC(create_find_dialog),
                            browser);
  
  gtk_widget_set_size_request (browser->OpenFindDialog, 16, 16);
  browser->OpenSaveAsBuntton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Save As",
                            "Save As",
                            "Save As",
                            browser->OpenSaveAsBuntton, 
                            GTK_SIGNAL_FUNC(create_save_document),
                            browser);
  
  gtk_widget_set_size_request (browser->OpenSaveAsBuntton, 16, 16);
  
  browser->OpenFromFileButton =
    gtk_toolbar_append_item(GTK_TOOLBAR(browser->ToolbarDown),
                            "Open From File",
                            "Open From File",
                            "Open From File",
                            browser->OpenFromFileButton, 
                            GTK_SIGNAL_FUNC(create_open_document_from_file),
                            browser);
  
  gtk_widget_set_size_request (browser->OpenFromFileButton, 16, 16);
  
  
}

/* METHODS THAT CREATE DIALOG FOR SPECIFICS TASK */


static void create_open_dialog(MinimoBrowser* browser)
{
  if (!browser) {
    quick_message("Creating the open dialog failed.");
    return;
  }
  
  GtkWidget *dialog;
  GtkWidget *dialog_vbox;
  GtkWidget *dialog_vbox2;
  GtkWidget *dialog_label;
  GtkWidget *dialog_comboentry;
  GtkWidget *dialog_area;
  GtkWidget *cancelbutton;
  GtkWidget *okbutton;
  GtkWidget *dialog_combo;
  GtkEntry *dialog_entry;
  
  dialog = gtk_dialog_new();
  
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(browser->topLevelWindow));
  
  /* set the position of this dialog immeditate below the toolbar */
  
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 320, 100);
  gtk_window_set_decorated (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
  
  gtk_window_set_title(GTK_WINDOW (dialog), "Open Dialog");
  
  dialog_vbox = GTK_DIALOG(dialog)->vbox;
  gtk_widget_show(dialog_vbox);
  
  dialog_vbox2 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(dialog_vbox2);
  gtk_box_pack_start(GTK_BOX (dialog_vbox), dialog_vbox2, TRUE, TRUE, 0);
  
  dialog_label = gtk_label_new("Enter a url:");
  gtk_widget_modify_font(dialog_label, getOrCreateDefaultMinimoFont());
  gtk_widget_show(dialog_label);
  gtk_box_pack_start(GTK_BOX (dialog_vbox2), dialog_label, FALSE, FALSE, 0);
  
  dialog_comboentry = gtk_combo_box_entry_new_text ();
  
  dialog_combo = gtk_combo_new ();
  dialog_entry = GTK_ENTRY (GTK_COMBO (dialog_combo)->entry);
  gtk_widget_modify_font(GTK_COMBO (dialog_combo)->entry, getOrCreateDefaultMinimoFont());
  gtk_widget_show(dialog_combo);
  gtk_box_pack_start(GTK_BOX (dialog_vbox2), dialog_combo, TRUE, TRUE, 0);
  
  populate_autocomplete((GtkCombo*) dialog_combo);
  
  /* setting up entry completion */
  build_entry_completion (browser);
  gtk_entry_set_completion (dialog_entry, minimo_entry_completion);
  g_object_unref (minimo_entry_completion);
  
  dialog_area = GTK_DIALOG(dialog)->action_area;
  gtk_widget_show(dialog_area);
  gtk_button_box_set_layout(GTK_BUTTON_BOX (dialog_area), GTK_BUTTONBOX_END);
  
  cancelbutton = gtk_button_new_with_label("Cancel");
  gtk_widget_modify_font(GTK_BIN(cancelbutton)->child, getOrCreateDefaultMinimoFont());
  gtk_widget_show(cancelbutton);
  gtk_dialog_add_action_widget(GTK_DIALOG (dialog), cancelbutton, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton, GTK_CAN_DEFAULT);
  
  okbutton = gtk_button_new_with_label("OK");
  gtk_widget_modify_font(GTK_BIN(okbutton)->child, getOrCreateDefaultMinimoFont());
  gtk_widget_show(okbutton);
  gtk_dialog_add_action_widget(GTK_DIALOG (dialog), okbutton, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton, GTK_CAN_DEFAULT);
  
  OpenDialogParams* dialogParams = (OpenDialogParams*) malloc(sizeof(OpenDialogParams));
  
  dialogParams->dialog_combo = dialog_combo;
  dialogParams->browser = browser;
  
  gtk_signal_connect (GTK_OBJECT (okbutton), "clicked", GTK_SIGNAL_FUNC (open_dialog_ok_cb), dialogParams);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (dialog_combo)->entry), "activate",GTK_SIGNAL_FUNC (open_dialog_ok_cb), dialogParams);
  gtk_signal_connect_object(GTK_OBJECT(okbutton), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  gtk_signal_connect_object(GTK_OBJECT(GTK_COMBO (dialog_combo)->entry), "activate", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  
  gtk_signal_connect_object(GTK_OBJECT(cancelbutton), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), dialog);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy", GTK_SIGNAL_FUNC(destroy_dialog_params_cb), dialogParams);
  
  gtk_widget_show_all(dialog);
}


void create_info_dialog (MinimoBrowser* browser)
{
  if (!browser) {
    quick_message("Creating the info dialog failed.");
    return;
  }
  
  char* titleString    = gtk_moz_embed_get_title((GtkMozEmbed*)browser->mozEmbed);
  char* locationString = gtk_moz_embed_get_location((GtkMozEmbed*)browser->mozEmbed);
  
  // first extract all of the interesting info out of the browser
  // object.
  
  GtkWidget *InfoDialog;
  GtkWidget *dialog_vbox5;
  GtkWidget *vbox4;
  GtkWidget *frame1;
  GtkWidget *table2;
  GtkWidget *URL;
  GtkWidget *Type;
  GtkWidget *RenderMode;
  GtkWidget *Encoding;
  GtkWidget *Size;
  GtkWidget *TypeEntry;
  GtkWidget *URLEntry;
  GtkWidget *ModeEntry;
  GtkWidget *SizeEntry;
  GtkWidget *PageTitle;
  GtkWidget *frame2;
  GtkWidget *SecurityText;
  GtkWidget *label10;
  GtkWidget *dialog_action_area5;
  GtkWidget *closebutton1;
  
  InfoDialog = gtk_dialog_new ();
  gtk_widget_set_size_request (InfoDialog, 240, 320);
  gtk_window_set_title (GTK_WINDOW (InfoDialog), ("Page Info"));
  gtk_window_set_position (GTK_WINDOW (InfoDialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (InfoDialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (InfoDialog), 240, 320);
  gtk_window_set_resizable (GTK_WINDOW (InfoDialog), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (InfoDialog), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (InfoDialog), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (InfoDialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (InfoDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_gravity (GTK_WINDOW (InfoDialog), GDK_GRAVITY_NORTH_EAST);
  gtk_window_set_transient_for(GTK_WINDOW(InfoDialog), GTK_WINDOW(browser->topLevelWindow));
  
  
  dialog_vbox5 = GTK_DIALOG (InfoDialog)->vbox;
  gtk_widget_show (dialog_vbox5);
  
  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox4);
  gtk_box_pack_start (GTK_BOX (dialog_vbox5), vbox4, TRUE, TRUE, 0);
  
  frame1 = gtk_frame_new (NULL);
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (vbox4), frame1, TRUE, TRUE, 0);
  
  table2 = gtk_table_new (5, 2, FALSE);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame1), table2);
  gtk_widget_set_size_request (table2, 240, -1);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 1);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 10);
  
  URL = gtk_label_new (("URL:"));
  gtk_widget_modify_font(URL, getOrCreateDefaultMinimoFont());
  gtk_widget_show (URL);
  gtk_table_attach (GTK_TABLE (table2), URL, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (URL), 0, 0.5);
  
  Type = gtk_label_new (("Type:"));
  gtk_widget_modify_font(Type, getOrCreateDefaultMinimoFont());
  gtk_widget_show (Type);
  gtk_table_attach (GTK_TABLE (table2), Type, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (Type), 0, 0.5);
  
  RenderMode = gtk_label_new (("Mode:"));
  gtk_widget_modify_font(RenderMode, getOrCreateDefaultMinimoFont());
  gtk_widget_show (RenderMode);
  gtk_table_attach (GTK_TABLE (table2), RenderMode, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (RenderMode), 0, 0.5);
  
  Encoding = gtk_label_new (("Encoding:"));
  gtk_widget_modify_font(Encoding, getOrCreateDefaultMinimoFont());
  gtk_widget_show (Encoding);
  gtk_table_attach (GTK_TABLE (table2), Encoding, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (Encoding), 0, 0.5);
  
  Size = gtk_label_new (("Size:"));
  gtk_widget_modify_font(Size, getOrCreateDefaultMinimoFont());
  gtk_widget_show (Size);
  gtk_table_attach (GTK_TABLE (table2), Size, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (Size), 0, 0.5);
  
  TypeEntry = gtk_entry_new ();
  gtk_widget_modify_font(TypeEntry, getOrCreateDefaultMinimoFont());
  gtk_widget_show (TypeEntry);
  gtk_table_attach (GTK_TABLE (table2), TypeEntry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable (GTK_EDITABLE (TypeEntry), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (TypeEntry), FALSE);
  
  URLEntry = gtk_entry_new ();
  gtk_widget_modify_font(URLEntry, getOrCreateDefaultMinimoFont());
  
  gtk_entry_set_text(GTK_ENTRY(URLEntry), locationString);
  
  gtk_widget_show (URLEntry);
  gtk_table_attach (GTK_TABLE (table2), URLEntry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable (GTK_EDITABLE (URLEntry), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (URLEntry), FALSE);
  
  ModeEntry = gtk_entry_new ();
  gtk_widget_modify_font(ModeEntry, getOrCreateDefaultMinimoFont());
  gtk_widget_show (ModeEntry);
  gtk_table_attach (GTK_TABLE (table2), ModeEntry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable (GTK_EDITABLE (ModeEntry), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (ModeEntry), FALSE);
  
  Encoding = gtk_entry_new ();
  gtk_widget_modify_font(Encoding, getOrCreateDefaultMinimoFont());
  gtk_widget_show (Encoding);
  gtk_table_attach (GTK_TABLE (table2), Encoding, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable (GTK_EDITABLE (Encoding), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (Encoding), FALSE);
  
  SizeEntry = gtk_entry_new ();
  gtk_widget_modify_font(SizeEntry, getOrCreateDefaultMinimoFont());
  gtk_widget_show (SizeEntry);
  gtk_table_attach (GTK_TABLE (table2), SizeEntry, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable (GTK_EDITABLE (SizeEntry), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (SizeEntry), FALSE);
  
  
  char buffer[50];
  sprintf(buffer, "%d bytes", browser->totalBytes);
  gtk_entry_set_text(GTK_ENTRY(SizeEntry), buffer);
  
  
  PageTitle = gtk_label_new ("");
  gtk_widget_modify_font(PageTitle, getOrCreateDefaultMinimoFont());
  
  if (titleString)
    gtk_label_set_text(GTK_LABEL(PageTitle), titleString);
  else
    gtk_label_set_text(GTK_LABEL(PageTitle), "Untitled Page");
  
  gtk_widget_show (PageTitle);
  gtk_frame_set_label_widget (GTK_FRAME (frame1), PageTitle);
  
  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox4), frame2, TRUE, TRUE, 0);
  
  SecurityText = gtk_label_new ("");
  gtk_widget_modify_font(SecurityText, getOrCreateDefaultMinimoFont());
  gtk_widget_show (SecurityText);
  gtk_container_add (GTK_CONTAINER (frame2), SecurityText);
  
  label10 = gtk_label_new (("Security Information"));
  gtk_widget_modify_font(label10, getOrCreateDefaultMinimoFont());
  gtk_widget_show (label10);
  gtk_frame_set_label_widget (GTK_FRAME (frame2), label10);
  
  dialog_action_area5 = GTK_DIALOG (InfoDialog)->action_area;
  gtk_widget_show (dialog_action_area5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area5), GTK_BUTTONBOX_END);
  
  //  closebutton1 = gtk_button_new_from_stock ("gtk-close");
  closebutton1 = gtk_button_new_with_label("Close");
  gtk_widget_modify_font(GTK_BIN(closebutton1)->child, getOrCreateDefaultMinimoFont());
  gtk_widget_show (closebutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (InfoDialog), closebutton1, GTK_RESPONSE_CLOSE);
  GTK_WIDGET_SET_FLAGS (closebutton1, GTK_CAN_DEFAULT);
  
  
  gtk_signal_connect_object(GTK_OBJECT(closebutton1), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), InfoDialog);
  
  gtk_widget_show_all(InfoDialog);
  
  free(titleString);
  free(locationString);
}

/* method to enable find a substring in the embed */
void
create_find_dialog (GtkWidget *w, MinimoBrowser *browser) {
  
  GtkWidget *window, *hbox, *entry, *label, *button, *ignore_case,
    *vbox_l, *hbox_tog;
  GList *glist = NULL;
  
  if (last_find) glist = g_list_append(glist, last_find);
  window = gtk_dialog_new();
  
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_keep_above(GTK_WINDOW (window), TRUE);
  gtk_widget_set_size_request (window, 230, 110);
  gtk_window_set_resizable(GTK_WINDOW (window),FALSE);
  gtk_window_set_title (GTK_WINDOW(window), "Find");
  setup_escape_key_handler(window);
  
  g_object_set_data(G_OBJECT(window), "Minimo", browser);
  g_object_set_data(G_OBJECT(window), "Embed", browser->mozEmbed);
  
  hbox = gtk_hbox_new (FALSE, 0);
  hbox_tog = gtk_hbox_new(FALSE, 0);
  vbox_l = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(hbox), 4);
  
  label = gtk_label_new ("Find: ");
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
  
  entry = gtk_combo_new ();
  gtk_combo_disable_activate(GTK_COMBO(entry));
  gtk_combo_set_case_sensitive(GTK_COMBO(entry), TRUE);
  if (last_find) gtk_combo_set_popdown_strings(GTK_COMBO(entry),glist);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(entry)->entry), "");
  g_object_set_data(G_OBJECT(window), "entry", entry);
  g_signal_connect(G_OBJECT(GTK_COMBO(entry)->entry), "activate", G_CALLBACK(find_dialog_ok_cb), window);
  gtk_box_pack_start (GTK_BOX(hbox), entry, FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
  
  ignore_case = gtk_check_button_new_with_label ("Ignore case");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(ignore_case), TRUE);
  g_object_set_data(G_OBJECT(window), "ignore_case", ignore_case);
  gtk_box_pack_start(GTK_BOX(vbox_l), ignore_case, FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(hbox_tog), vbox_l, FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox_tog, FALSE, FALSE, 0);
  
  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(window)->action_area), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(find_dialog_ok_cb), window);
  
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(window)->action_area), button, FALSE, FALSE, 0);
  g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_destroy), (gpointer)window);
  
  gtk_widget_show_all (window);
  gtk_widget_grab_focus (entry);     	
}

/* Method that create the "save dialog" */
void 
create_save_document(GtkButton *button, MinimoBrowser *browser) 
{
  GtkWidget *fs, *all, *ok_button, *cancel_button, *hbox;
  GtkWidget *SaveDialog, *scrolled_window;
  
  G_CONST_RETURN gchar *location = NULL, *file_name = NULL;
  
  g_return_if_fail(browser->mozEmbed != NULL);
  
  location = gtk_moz_embed_get_location(GTK_MOZ_EMBED (browser->mozEmbed));
  if (location) file_name = g_basename(location);
  all = gtk_check_button_new_with_label("Save everything, including images and framesets?");
  
  fs = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_SAVE);
  SaveDialog = gtk_dialog_new ();
  gtk_widget_set_size_request (SaveDialog, 240, 320);
  gtk_window_set_title (GTK_WINDOW (SaveDialog), ("Save as"));
  gtk_window_set_position (GTK_WINDOW (SaveDialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (SaveDialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (SaveDialog), 240, 320);
  gtk_window_set_resizable (GTK_WINDOW (SaveDialog), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (SaveDialog), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (SaveDialog), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (SaveDialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (SaveDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_gravity (GTK_WINDOW (SaveDialog), GDK_GRAVITY_NORTH_EAST);
  gtk_window_set_transient_for(GTK_WINDOW(SaveDialog), GTK_WINDOW(browser->topLevelWindow));
  
  scrolled_window = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_show(scrolled_window);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG (SaveDialog)->vbox),scrolled_window,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  
  gtk_scrolled_window_add_with_viewport ( GTK_SCROLLED_WINDOW (scrolled_window) ,fs);
  
  g_object_set_data(G_OBJECT(fs), "minimo", browser);
  g_object_set_data(G_OBJECT(fs), "save_all", all);
  g_object_set_data(G_OBJECT(fs), "save_dialog", SaveDialog);
  
  /* adding extra button into the widget -> 'Ok and Cancel' Button */
  ok_button = gtk_button_new_with_label ("Ok");
  gtk_widget_modify_font(GTK_BIN(ok_button)->child, getOrCreateDefaultMinimoFont());
  
  cancel_button = gtk_button_new_with_label ("Cancel");
  gtk_widget_modify_font(GTK_BIN(cancel_button)->child, getOrCreateDefaultMinimoFont());
  
  all = gtk_check_button_new_with_label("Save everything, including images and framesets?");
  
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX (hbox), ok_button, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX (hbox), cancel_button, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX (hbox), all, FALSE, TRUE, 10);
  gtk_widget_show_all (hbox);
  
  gtk_file_chooser_set_extra_widget ( GTK_FILE_CHOOSER (fs), GTK_WIDGET (hbox));
  
  /* connecting callbacks into the extra buttons */
  g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(on_save_ok_cb), fs);
  g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), (gpointer) SaveDialog);
  
  gtk_widget_show(fs);
  gtk_widget_show_all(SaveDialog);
  
  return;
}

/* Method that create the "Open From File .." Dialog */
void
create_open_document_from_file (GtkButton *button, MinimoBrowser *browser) {
  
  GtkWidget *fs, *ok_button, *cancel_button, *hbox;
  GtkWidget *OpenFromFileDialog, *scrolled_window;
  
  G_CONST_RETURN gchar *location = NULL, *file_name = NULL;
  
  g_return_if_fail(browser->mozEmbed != NULL);
  
  location = gtk_moz_embed_get_location(GTK_MOZ_EMBED (browser->mozEmbed));
  if (location) file_name = g_basename(location);
  
  fs = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
  OpenFromFileDialog = gtk_dialog_new ();
  gtk_widget_set_size_request (OpenFromFileDialog, 240, 320);
  gtk_window_set_title (GTK_WINDOW (OpenFromFileDialog), ("Open URL From File"));
  gtk_window_set_position (GTK_WINDOW (OpenFromFileDialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (OpenFromFileDialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (OpenFromFileDialog), 240, 320);
  gtk_window_set_resizable (GTK_WINDOW (OpenFromFileDialog), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (OpenFromFileDialog), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (OpenFromFileDialog), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (OpenFromFileDialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (OpenFromFileDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_gravity (GTK_WINDOW (OpenFromFileDialog), GDK_GRAVITY_NORTH_EAST);
  gtk_window_set_transient_for(GTK_WINDOW(OpenFromFileDialog), GTK_WINDOW(browser->topLevelWindow));
  
  scrolled_window = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_show(scrolled_window);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG (OpenFromFileDialog)->vbox),scrolled_window,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  
  gtk_scrolled_window_add_with_viewport ( GTK_SCROLLED_WINDOW (scrolled_window) ,fs);
  
  g_object_set_data(G_OBJECT(fs), "minimo", browser);
  g_object_set_data(G_OBJECT(fs), "open_dialog", OpenFromFileDialog);
  
  /* adding extra button into the widget -> 'Ok and Cancel' Button */
  ok_button = gtk_button_new_with_label ("Ok");
  gtk_widget_modify_font(GTK_BIN(ok_button)->child, getOrCreateDefaultMinimoFont());
  
  cancel_button = gtk_button_new_with_label ("Cancel");
  gtk_widget_modify_font(GTK_BIN(cancel_button)->child, getOrCreateDefaultMinimoFont());
  
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX (hbox), ok_button, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX (hbox), cancel_button, FALSE, TRUE, 10);
  gtk_widget_show_all (hbox);
  
  gtk_file_chooser_set_extra_widget ( GTK_FILE_CHOOSER (fs), GTK_WIDGET (hbox));
  
  /* connecting callbacks into the extra buttons */
  g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(on_open_ok_cb), fs);
  g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), (gpointer) OpenFromFileDialog		);
  
  gtk_widget_show(fs);
  gtk_widget_show_all(OpenFromFileDialog);
  
  return;
}

/* METHODS TO MANIPULATE TABS */

static GtkWidget *get_active_tab (MinimoBrowser *browser)
{
  GtkWidget *active_page;
  gint current_page;
  
  current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
  active_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), current_page);
  
  return active_page;
}

static void open_new_tab_cb (GtkButton *button, MinimoBrowser *browser)
{
  GtkWidget *page_label;
  gint num_pages;
  gchar *page_title;
  gint current_page;
  
  g_return_if_fail(browser->notebook != NULL);
  
  num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
  page_title = g_strdup_printf("%d",num_pages+1);
  page_label = gtk_label_new (page_title);
  
  browser->mozEmbed = gtk_moz_embed_new();
  
  gtk_notebook_append_page (GTK_NOTEBOOK (browser->notebook), browser->mozEmbed, page_label);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (browser->notebook), browser->mozEmbed, FALSE, FALSE, GTK_PACK_START);
  gtk_widget_show (browser->mozEmbed);
  
  current_page = gtk_notebook_page_num (GTK_NOTEBOOK (browser->notebook), browser->mozEmbed);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), current_page);
  browser->active_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), current_page);
  
  num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
  
  /* applying the Signal into the new Render Engine - MozEmbed */
  
  // hook up the title change to update the window title
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "title",       GTK_SIGNAL_FUNC (title_changed_cb), browser);
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "location",    GTK_SIGNAL_FUNC(location_changed_cb), page_label);
  
  // hook up the start and stop signals
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "net_start",   GTK_SIGNAL_FUNC(load_started_cb), browser);
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "net_stop",    GTK_SIGNAL_FUNC(load_finished_cb), browser);
  
  // hookup to see whenever a new window is requested
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "new_window",  GTK_SIGNAL_FUNC(new_window_cb), browser);
  
  // hookup to any requested visibility changes
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "visibility",     GTK_SIGNAL_FUNC(visibility_cb), browser);
  
  // hookup to the signal that is called when someone clicks on a link to load a new uri
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "open_uri",    GTK_SIGNAL_FUNC(open_uri_cb), browser);
  
  // this signal is emitted when there's a request to change the containing browser window to a certain height, like with width
  // and height args for a window.open in javascript
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "size_to",     GTK_SIGNAL_FUNC(size_to_cb), browser);
  gtk_signal_connect (GTK_OBJECT  (browser->mozEmbed), "link_message",GTK_SIGNAL_FUNC (link_message_cb), browser);
  
  num_browsers= num_pages;
}

static void close_current_tab_cb (GtkMenuItem *menuitem, MinimoBrowser *browser)
{
  gint current_page, num_pages;
  
  num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
  
  if (num_pages >1) {
    
  	current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
    
  	gtk_notebook_remove_page (GTK_NOTEBOOK (browser->notebook), current_page);
  	num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
	num_browsers= num_pages;
    
  	if (num_pages == 1) {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (browser->notebook), FALSE);
	}
  }
}

static void switch_page_cb (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, MinimoBrowser *browser)
{
  browser->active_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), page_num);
  update_ui (browser);
  
}

static void show_hide_tabs_cb (GtkButton *button, MinimoBrowser *browser)
{
  if (browser->show_tabs) {
  	gtk_notebook_set_show_tabs (GTK_NOTEBOOK(browser->notebook), FALSE);
  	browser->show_tabs = FALSE;
  }
  else {
  	gtk_notebook_set_show_tabs (GTK_NOTEBOOK(browser->notebook), TRUE);
  	browser->show_tabs = TRUE;
  }
  
}


/* CALLBACKS METHODS ... */

static void on_open_ok_cb (GtkWidget *button, GtkWidget *fs) {
  MinimoBrowser *browser = NULL;
  G_CONST_RETURN gchar *filename;
  gchar *converted;
  
  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fs));
  
  if (!filename || !strcmp(filename,"")) {
    
    return ;
    
  } else {
    
    converted = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
    
    browser = (MinimoBrowser *) g_object_get_data(G_OBJECT(fs), "minimo");
    
    if (converted != NULL) {
      
      gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser->mozEmbed), converted);
      gtk_widget_grab_focus (GTK_WIDGET(browser->mozEmbed));
    }
	
    g_free (converted);
    g_free ((void *) filename);
    gtk_widget_destroy((GtkWidget *) g_object_get_data(G_OBJECT(fs), "open_dialog"));
    
  }
  
  return ;
}

static void on_save_ok_cb(GtkWidget *button, GtkWidget *fs) {
  MinimoBrowser *browser = NULL;
  G_CONST_RETURN gchar *filename;
  
  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fs));
  
  if (!filename || !strcmp(filename,"")) {
    
    return ;
    
  } else {
    browser = (MinimoBrowser *) g_object_get_data(G_OBJECT(fs), "minimo");
    
    if (!mozilla_save( GTK_MOZ_EMBED (browser->mozEmbed), (gchar *) filename, TRUE )) {
      //create_dialog("Error","Save failed, did you enter a name?",0,0);
      
    }
    
    gtk_widget_destroy((GtkWidget *) g_object_get_data(G_OBJECT(fs), "save_dialog"));
    gtk_widget_destroy(fs);
  }
  
  return ;
}

static void find_dialog_ok_cb (GtkWidget *w, GtkWidget *window)
{
  
  GtkWidget *entry = (GtkWidget *)g_object_get_data(G_OBJECT(window), "entry");
  
#define	GET_FIND_OPTION(window, name)	\
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(window), name)))
  
  browser->didFind = mozilla_find( GTK_MOZ_EMBED (browser->mozEmbed),
                                   gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(entry)->entry)),
                                   !GET_FIND_OPTION(window, "ignore_case"),
                                   FALSE, //GET_FIND_OPTION(window, "backwards"),
                                   TRUE, //GET_FIND_OPTION(window, "auto_wrap"),
                                   FALSE, //GET_FIND_OPTION(window, "entire_word"),
                                   TRUE, //GET_FIND_OPTION(window, "in_frame"),
                                   FALSE);
  
  //	if (!browser->didFind) create_dialog("Find Results", "Expression was not found!", 0, 1);
  if (last_find) g_free(last_find);
  last_find = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(entry)->entry)));
}

/* OTHERS METHODS */

static void update_ui (MinimoBrowser *browser)
{
  gchar *tmp;
  int x=0;
  char *url;
  
  tmp = gtk_moz_embed_get_title (GTK_MOZ_EMBED (browser->active_page));
  
  if (tmp) x = strlen (tmp);
  if (x == 0) tmp = "Minimo";
  gtk_window_set_title (GTK_WINDOW(browser->topLevelWindow), tmp);
  url = gtk_moz_embed_get_location (GTK_MOZ_EMBED (browser->active_page));
  
}

/* Method that build the entry completion */
void build_entry_completion (MinimoBrowser* browser) {
  
  /* Minimo entry completion */
  minimo_entry_completion = gtk_entry_completion_new ();
  
  gtk_entry_completion_set_model (minimo_entry_completion, GTK_TREE_MODEL (store));
  gtk_entry_completion_set_text_column (minimo_entry_completion, 0);
}

/* Method to sep up the escape key handler */
void setup_escape_key_handler(GtkWidget *window) {	
  g_signal_connect(G_OBJECT(window), "key_press_event",
                   G_CALLBACK(escape_key_handler), NULL);
}

/* Method to handler the escape key */
gint escape_key_handler(GtkWidget *window, GdkEventKey *ev) {
  g_return_val_if_fail(window != NULL, FALSE);
  if (ev->keyval == GDK_Escape) {
    gtk_widget_destroy(window);
    return (1);
  }
  return (0);
}
