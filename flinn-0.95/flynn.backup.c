/*
Flynn - v0.94 / 2001-10-14
adam@gimp.org
*/

#ifdef USE_JOYSTICK
#  include <fcntl.h>
#  include <linux/joystick.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "readppm.h"
#include "readppm.c" /* hm. */



/* Window dimensions. */
#define WWIDTH (1024)
#define WHEIGHT (768)
/*
#define WWIDTH 1136
#define WHEIGHT 852
*/

/* Dimensions of the PPM files accepted. */
#define PPM_WIDTH (154)
#define PPM_HEIGHT (106)

/* The greater this number, the slower the scrolling
inertia.  1 == instantaneous.  */
#define SLIDE_FRICTION (10)

/* Distances between the rows and columns. */
#define X_PAD (50)
#define Y_PAD (50)

/* Number of columns. */
#define COLUMNS (5)

/* The amount and speed by which each thumbnail wiggles in-place... */
#define JIGGLE (10.0F)
#define JIGGLE_SPEED (1.0F)

#define SPARK_DIST (50.0F)
#define SPARK_OUT_SPEED (1.0F)
#define NUM_SPARK (3)
#define SPARK_SPEED (0.14F)
#define CONSTRAIN_SPEED (0.04F)

/* Number of seconds of inactivity before sleeping. */
#define IDLE_MSECS 600 * 1000

static GtkWidget* drawing_area;
static GtkWidget* eventbox;

static guint idle_id = 0;
static guint timeout_id = 0;

static guchar* disp;

int iterate(void* whatever);

typedef struct {
  char *mnemonic;
  char *ppm_name;
  char *conf_name;
  char *exec_cmd;
  unsigned char *ppm_data;
  int ppm_loaded;
} gameStruct;


typedef struct {
  int ox, oy;
  int gx, gy;
} flynnStateStruct;

flynnStateStruct flynn_state = {0,0,0,0};
int state_init = 0;
gameStruct game[4000];
int num_games = 0;
int selected = -1;
char *arg0;
char *arg1;
char cwd[999];
int joy_wait;

struct itimerval timerval;

void save_state(void)
{
  FILE *fp;

  fp = fopen("/tmp/flynn.state", "wb");

  fprintf(fp, "%d %d %d %d",
	  flynn_state.ox,
	  flynn_state.oy,
	  flynn_state.gx,
	  flynn_state.gy
	  );

  fclose(fp);
}


void load_state(void)
{
  FILE *fp;

  fp = fopen("/tmp/flynn.state", "rb");
  if (!fp)
    {
      return;
    }

  fscanf(fp, "%d %d %d %d",
	  &flynn_state.ox,
	  &flynn_state.oy,
	  &flynn_state.gx,
	  &flynn_state.gy
	  );

  fclose(fp);
}


void flynn_exit(int rtn)
{
  save_state();
  exit(rtn);
}


void run_game(void)
{
  save_state();

  if (selected > -1 && selected < num_games)
    {
      execl(game[selected].exec_cmd, "your_arg0", game[selected].mnemonic, arg0, arg1, NULL);
    }
}


void idle_stop(void)
{
  if (idle_id != 0)
    {
      gtk_idle_remove(idle_id);
      idle_id = 0;
    }  
}


void idle_restart(void)
{
  idle_stop();
  idle_id = gtk_idle_add (iterate, NULL);
}


gint timeout_trigger(gpointer unused)
{
  idle_stop();

  timeout_id = 0;
  return (0);
}


void timeout_restart(void)
{
  idle_restart();

  if (timeout_id != 0)
    {
      gtk_timeout_remove(timeout_id);
    }

  timeout_id = gtk_timeout_add(IDLE_MSECS,
			       timeout_trigger,
			       NULL);
}


#ifdef USE_JOYSTICK
int joy_fd;
struct js_event joy_event;

void set_timer() {
  timerval.it_interval.tv_sec = 0;
  timerval.it_value.tv_usec = 200000;
  setitimer(ITIMER_VIRTUAL,&timerval,&timerval);
}

void proc_joy_event() {
  timeout_restart();

  switch (joy_event.type) {
           case JS_EVENT_AXIS:
               if (joy_event.number == 0) {
                       if (joy_event.value < 0) {
                               /* left */
                               if (flynn_state.gx > 0) {
				 if (joy_wait != 1) {
				   joy_wait = 1;
				   flynn_state.gx--;
				   set_timer();
				 }
			       }
                       } 
		       else if (joy_event.value > 0) {
                               /* right */
                               if (flynn_state.gx < COLUMNS-1) {
                                 if (joy_wait != 1) {
				   joy_wait = 1;	 
				   flynn_state.gx++;
				   set_timer();
				 }
			       }
                       }
               } 
	       else if (joy_event.number == 1) {
                       if (joy_event.value < 0) {
                               /* up */
                               if (flynn_state.gy > 0) {
                                 if (joy_wait != 1) {
				    joy_wait = 1;	 
				    flynn_state.gy--;
				    set_timer();
				 }
			       }
                       } 
		       else if (joy_event.value > 0) {
                               /* down */
                               if (flynn_state.gy < (num_games+COLUMNS-1)/COLUMNS -1) {
				 if (joy_wait != 1) {
				   joy_wait = 1;
                                   flynn_state.gy++;
				   set_timer();
				 }
		               }
                       }
               }
               break;
           case JS_EVENT_BUTTON:
	       //printf("button %d pressed\n",joy_event.number);
	       if (joy_event.number == 0) {
		  arg0 = "-ror";    
	       }
	       else {
		  arg0 = "";
	       }
	       run_game();
               break;
           default:
               break;
    }
}
#endif


gint key_press(GtkWidget *widget,
	       GdkEvent  *event)
{
  GdkEventKey    *kevent;

  timeout_restart();

  kevent = (GdkEventKey *) event;
  switch (kevent->keyval)
    {
    case GDK_Escape:
      /*      system("(~/xv&sleep 10;killall xv)&"); */
      /* exit(-2); */
      flynn_exit(-2);
      break;

    case GDK_Down:
      if (flynn_state.gy < (num_games+COLUMNS-1)/COLUMNS -1)
	flynn_state.gy++;
      break;

    case GDK_Up:
      if (flynn_state.gy > 0)
	flynn_state.gy--;
      break;

    case GDK_Left:
      if (flynn_state.gx > 0)
	flynn_state.gx--;
      break;

    case GDK_Right:
      if (flynn_state.gx < COLUMNS-1)
	flynn_state.gx++;
      break;

    case GDK_Shift_L:
    case GDK_Control_L:
    case GDK_Alt_L:
    case GDK_space:
    case GDK_z:
    case GDK_x:
    case GDK_1:
    case GDK_2:
    case GDK_5:
    case GDK_6:
    default:
      arg0 = "";
      run_game();
      break;
    }

  return FALSE;
}


void init_work(char *base_dir)
{
  FILE *fp;
  char *fn;
  char line[100];
  int i;

  load_state();

  /* read config file */
  fprintf(stderr, "reading flynn config tree from directory %s\n", base_dir);

  chdir(base_dir);

  fn = g_strconcat("gamelist", NULL);
  fp = fopen(fn, "rb");
  g_free(fn);

  if (!fp)
    {
      fprintf(stderr, "could not open game list file.\n");
      flynn_exit(-1);
    }

  while (fgets(line, 100, fp))
    {
      int len;
      
      if (!(len = strlen(line)))
	continue;

      if (line[0] == '#')
	continue;

      for (i=0; i<len; i++)
	if (line[i] != '\n' &&
	    line[i] != '\t' &&
	    line[i] != ' ')
	  goto found_non_whitespace;

      continue;

    found_non_whitespace:

      line[--len] = '\0';
      /* fprintf(stderr, "adding game %s\n", line); */

      game[num_games].mnemonic = g_strdup(line);
      game[num_games].ppm_name = g_strconcat("games/", line, ".ppm", NULL);
      game[num_games].conf_name = g_strconcat("games/", line, ".conf", NULL);
      game[num_games].exec_cmd = g_strconcat("./run_game", NULL);
      game[num_games].ppm_loaded = 0;

      num_games++;
    }

  fclose(fp);

  for (i=0; i<num_games; i++)
    {
      int w, h, r;
      PnmPixformat pf = PNM_RGB;

      r = read_pnm(game[i].ppm_name, &game[i].ppm_data, &w, &h, &pf);

      fn = g_strconcat("default.ppm", NULL);

      if (r)
	r = read_pnm(fn, &game[i].ppm_data, &w, &h, &pf);

      if (w != PPM_WIDTH ||
	  h != PPM_HEIGHT)
	{
	  if (game[i].ppm_data)
	    {
	      free(game[i].ppm_data);
	      game[i].ppm_data = NULL;
	    }
	}

      if (game[i].ppm_data && !r)
	game[i].ppm_loaded = 1;	  
      else
	fprintf(stderr, "ppm for %s is missing or inappropriate\n", game[i].mnemonic);
    }

  idle_restart();
  timeout_restart();
}


void init_ui(void)
{
  int argc;
  char **argv;
  GtkWidget* dlg;


  disp = malloc(WWIDTH*WHEIGHT*3);


  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("myprog");
  gtk_init (&argc, &argv);

  
  gdk_rgb_init ();
  gtk_widget_set_default_visual (gdk_rgb_get_visual());
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap());


  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) gtk_main_quit,
                      NULL);
  gtk_window_set_title (GTK_WINDOW (dlg), "FLYNN");
  {
    {
      eventbox = gtk_event_box_new();
      gtk_container_border_width (GTK_CONTAINER (eventbox), 0);
      gtk_container_add (GTK_CONTAINER (dlg), GTK_WIDGET (eventbox));
      {
	drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_usize (drawing_area, WWIDTH, WHEIGHT);
	gtk_container_add (GTK_CONTAINER (eventbox),
			   GTK_WIDGET (drawing_area));
	
	gtk_widget_show (drawing_area);
      }
      gtk_widget_show (eventbox);
    }
  }
  gtk_widget_add_events (dlg,
			 GDK_KEY_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (dlg), "key_press_event",
		      (GtkSignalFunc) key_press,
		      NULL);
  gtk_widget_show (dlg);

  /* gdk_key_repeat_disable (); */
/* gdk_keyboard_grab (eventbox->window, FALSE, time); */
}


void show(void)
{
  gdk_draw_rgb_image (drawing_area->window,
			 drawing_area->style->white_gc,
			 0, 0, WWIDTH, WHEIGHT,
		      /* GDK_RGB_DITHER_NORMAL, */
		      GDK_RGB_DITHER_NONE,
		      /* GDK_RGB_DITHER_MAX, */
			 (guchar*)disp, WWIDTH*3);
  gdk_flush();
}


void put_rgb(unsigned char *data,
	     int data_w, int data_h,
	     int buf_w, int buf_h,
	     int x, int y)
{
  int i,j;

  if (x <= -data_w || y <= -data_h ||
      x >= buf_w || y >= buf_h)
    {
      return;
    }

  if (x>=0 && y>=0 && x+data_w<buf_w && y+data_h<buf_h)
    {
      for (j=0; j<data_h; j++)
	{
	  memcpy(&disp[3*(buf_w*(y+j) + x)],
		 &data[3*data_w * j],
		 3*data_w);
	}
      return;
    }

  if (x>=0 && x+data_w<buf_w)
    {
      for (j=MAX(0, -y); j<MIN(data_h, buf_h-y); j++)
	{
	  memcpy(&disp[3*(buf_w*(y+j) + x)],
		 &data[3*data_w * j],
		 3*data_w);
	}
      return;
    }

  for (j=0; j<data_h; j++)
    {
      for (i=0; i<data_w; i++)
	{
	  int rx;
	  rx = x+i;
	  if (rx>=0 && rx<buf_w)
	    {
	      int ry = y+j;
	      if (ry>=0 && ry<buf_h)
		{
		  disp[3*(buf_w*ry + rx)] = data[3*(data_w * j + i)];
		  disp[3*(buf_w*ry + rx)+1] = data[3*(data_w * j + i)+1];
		  disp[3*(buf_w*ry + rx)+2] = data[3*(data_w * j + i)+2];
		}
	    }
	}
    }
}


int iterate(void* whatever)
{
  double sx=0.0F, sy=0.0F;
  static double s1o=0.0F, s2o=0.0F, s3o=0.0F, s4o=0.0F, s5o=0.0F;
  static double spark_out = 0.0F;
  static double constrain = 0.0F;
  static int last_selected = -1;

#ifdef USE_JOYSTICK
  if (read(joy_fd, &joy_event, sizeof(struct js_event)) > 0)
       proc_joy_event();
#endif

  /* DO STUFF IN DISP HERE */
  {
    int i;
    int wox, woy;

    wox = (X_PAD + PPM_WIDTH) * flynn_state.gx - WWIDTH/2 + PPM_WIDTH/2;
    woy = (Y_PAD + PPM_HEIGHT) * flynn_state.gy - WHEIGHT/2 + PPM_HEIGHT/2;

    if (state_init)
      {
	flynn_state.ox = (flynn_state.ox*(SLIDE_FRICTION-1) + wox) / SLIDE_FRICTION;
	flynn_state.oy = (flynn_state.oy*(SLIDE_FRICTION-1) + woy) / SLIDE_FRICTION;
      }
    else
      {
	flynn_state.ox = wox;
	flynn_state.oy = woy;
	state_init = 1;
      }

    s1o += 0.029f;
    s2o -= 0.032f;

    /* draw the background bars. */
    for (i=0; i<WHEIGHT; i++)
      memset(&disp[3*WWIDTH*i],
	     fabs(23.0F +
		  64.0F*sin(((double)(i+flynn_state.oy/3.0F))/20.0F + s1o)
		  +
		  58.0F*cos(1.0 + ((double)(i+flynn_state.oy/2.0F))/155.0F + s2o)
		  ),
	     3*WWIDTH);

    s4o += 0.119F * JIGGLE_SPEED;
    s5o -= 0.140F * JIGGLE_SPEED;

    last_selected = selected;
    selected = -1;
    for (i=0; i<num_games; i++)
      {
	double dx, dy;

	dx = (i%COLUMNS) * (X_PAD + PPM_WIDTH) - flynn_state.ox;
	dy = (i/COLUMNS) * (Y_PAD + PPM_HEIGHT) - flynn_state.oy;

	if (i%COLUMNS == flynn_state.gx &&
	    i/COLUMNS == flynn_state.gy)
	  {
	    selected = i;
	    
	    if (selected != last_selected)
	      {
		spark_out = 0.0F;
		constrain = 1.0F;
	      }
	    else
	      {
		spark_out += SPARK_OUT_SPEED;
		constrain -= CONSTRAIN_SPEED;
		
		if (spark_out > SPARK_DIST)
		  spark_out = SPARK_DIST;
		
		if (constrain < 0.0F)
		  constrain = 0.0F;
	      }

	    dx += constrain *
	      (sin(s4o + 0.8F*((double)(i%COLUMNS + i/COLUMNS)) ) * JIGGLE);
	    dy += constrain *
	      (cos(s5o + 0.8F*((double)(i%COLUMNS - i/COLUMNS)) ) * JIGGLE);

	    sx = dx;
	    sy = dy;
	  }
	else
	  {
	    dx += (sin(s4o + 0.8F*((double)(i%COLUMNS + i/COLUMNS)) ) * JIGGLE);
	    dy += (cos(s5o + 0.8F*((double)(i%COLUMNS - i/COLUMNS)) ) * JIGGLE);

	    put_rgb(game[i].ppm_data, PPM_WIDTH, PPM_HEIGHT,
		    WWIDTH, WHEIGHT,
		    dx, dy
		    );
	  }
      }


    s3o += SPARK_SPEED;

    if (selected != -1)
      {
	/* draw orbital icons */
	for (i=0; i<NUM_SPARK; i++)
	  {
	    put_rgb(game[selected].ppm_data, PPM_WIDTH, PPM_HEIGHT,
		    WWIDTH, WHEIGHT,
		    sx + (spark_out * cos(s3o + i*2.0F*M_PI/(double)NUM_SPARK)),
		    sy + (spark_out * sin(s3o + i*2.0F*M_PI/(double)NUM_SPARK))
		    );
	  }
	
	/* draw selected icon on top */
	put_rgb(game[selected].ppm_data, PPM_WIDTH, PPM_HEIGHT,
		WWIDTH, WHEIGHT,
		sx, sy
		);
      }
  }  

  show();

  /* fprintf(stderr, "%s             \r", selected==-1 ?
     "(none)" : game[selected].mnemonic); */

  return (idle_id ? 1 : 0);
}

void joy_alarm(int sig_num) {
  joy_wait = 0;
}


int main(int argc, char *argv[])
{
  getcwd(cwd, 999);
  arg0 = argv[0];
  arg1 = argc ? argv[1] : NULL;

  /* LOAD FILES AND STUFF HERE */
  init_work(argc ? argv[1] : NULL);
  init_ui();

#ifdef USE_JOYSTICK
  printf("Initializing joystick...\n");
  signal(SIGVTALRM,joy_alarm);
  joy_fd = open ("/dev/input/js0", O_RDONLY | O_NONBLOCK);
#endif

  idle_id = gtk_idle_add (iterate, NULL);

  system("xset s noblank");
  system("xset s off");
  system("xset -dpms");
  
  /* -------------------------------------- */
  gtk_main();
  gdk_flush ();
  /* -------------------------------------- */

  system("xset +dpms");
  system("xset s on");
  system("xset s blank");
  
  flynn_exit(0);
}
