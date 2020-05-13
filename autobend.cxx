 /*
 * This file is part of autobend, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// Build with: 
// gcc autobend.c -o autobend -lfltk -lasound -lpthread -lstdc++

// TODO slider react to keys + - up dn pgup pgdn home end 0 (suppr?)
 
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Button.H>
// #include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_File_Chooser.H>


#include <alsa/asoundlib.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// from https://stackoverflow.com/questions/14769603/how-can-i-write-a-clamp-clip-bound-macro-for-returning-a-value-in-a-gi
#define CLAMP(x, low, high) ({\
  __typeof__(x) __x = (x); \
  __typeof__(low) __low = (low);\
  __typeof__(high) __high = (high);\
  __x > __high ? __high : (__x < __low ? __low : __x);\
  })

static void show_err ( const char * format, ... ){
	char s[1024];
	va_list args;
    va_start(args, format);
    vsnprintf(s, 1024, format, args);
    fprintf(stderr, "%s\n", s);
    fl_alert("%s", s);
    va_end(args);
}

int strtonote(const char* str, const char **endptr){
	int note_nums[]={9,11,0,2,4,5,7}; // A..G
	const char *ptr=str;
	char *ptr2;
	int note;
	while(*ptr==32) ptr++; // Skip leading spaces
	char c=toupper(*ptr);
	if(c>='A' && c<='G'){
		// got a proper note name
		note=note_nums[c-'A'];
		ptr++;
		// check for alteration (single or double)
		if (*ptr=='#') {ptr++; note++; if (*ptr=='#') {ptr++; note++;}}
		else if (*ptr=='x' || *ptr=='X') {ptr++; note+=2;}
		else if (*ptr=='b') {ptr++; note--; if (*ptr=='b') {ptr++; note--;}}
		note = note % 12; // Cbb -> -2
		// get octave
		/*
		octave=strtol(ptr, &ptr2, 10)+1;
		if(ptr2>ptr){
			note+=12*octave;
			if(note<0 or note>127){
				fprintf(stderr, "Note %u out of range\n", note);
				note=-1;
				ptr=str;
			}else{
				ptr=ptr2;
			}
		}else{
			fprintf(stderr, "Cannot convert \"%s\" to octave number\n", ptr);
			note=-1;
			ptr=str;
		}
		*/
	}else{
		// Illegal note name
		if (*ptr) fprintf(stderr, "Cannot convert '%c' (%u) to note\n", c, c);
		note=-1;
		ptr=str;
	}
	*endptr=ptr;
	return(note);
}
// FLTK
Fl_Double_Window *main_window=(Fl_Double_Window *)0;

class Note_Slider : public Fl_Value_Slider
{
protected:
/*
	static void event_callback(Fl_Widget*, void*);
*/
private:
//	void draw(int X, int Y, int W, int H);
//	void draw();

	int handle(int event);
public:
	Note_Slider(int x, int y, int w, int h, const char *l=0) : Fl_Value_Slider(x,y,w,h,l)
	{
//		callback(&event_callback, (void*)this);
//		end(); // ??
	}
	~Note_Slider() { }
};

int Note_Slider::handle(int event){
//	printf("Note_Slider::Handle!\n");
	double val, minval, maxval, step;
	val=this->value();
	minval=this->minimum();
	maxval=this->maximum();
	if (minval>maxval){
		minval=maxval;
		maxval=this->minimum();
	}
	step=1; // (maxval-minval)/100.0f; // ??
	// printf("key %u on %lu\n", Fl::event_key (), this->argument());
	// printf("xmin %f xmax %f step %f\n", minval, xmaxval, step);
	// printf("ymin %f ymax %f ystep %f\n", yminval, ymaxval, ystep);
	switch (event) 
	{
		// case FL_KEYUP:
		case FL_KEYBOARD: // It seems every control is getting that until handled ?
			if (Fl::focus() == this) {
				switch(Fl::event_key ()){
					case FL_Home:
						this->value(minval);
						break;
					case FL_End:
						this->value(maxval);
						break;
					case FL_KP+'+':
					// case FL_Up:
					// case FL_Right:
						val+=step;
						if (val>maxval) val=maxval;
						this->value(val);
						break;
					case FL_KP+'-':
					// case FL_Down:
					// case FL_Left:
						val-=step;
						if (val<minval) val=minval;
						this->value(val);
						break;
					case FL_KP+'.':
					case FL_Delete:
						val=0;
						if (val>maxval) val=maxval;
						if (val<minval) val=minval;
						this->value(val);
						break;
					case FL_Page_Up:
						val+=step*10;
						if (val>maxval) val=maxval;
						this->value(val);
						break;
					case FL_Page_Down:
						val-=step*10;
						if (val<minval) val=minval;
						this->value(val);
						break;
					default:
						return Fl_Value_Slider::handle(event);
				}
				this->set_changed(); // TODO Not always changed
				this->callback()((Fl_Widget*)this, 0);
				return 1;
			}
			break;
		case FL_FOCUS:
//			printf("Note_Slider::Focus %lu!\n", this->argument());
			Fl::focus(this);
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_UNFOCUS:
			// printf("Note_Slider::Unfocus %lu!\n", this->argument());
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_PUSH:
		/*
			if(Fl::event_clicks()){ // Double click or more
				// printf("Note_Slider::double click %lu!\n", this->argument());
				this->value(minval);
				this->yvalue(yminval);
				this->callback()((Fl_Widget*)this, 0);
				return 1; // Don't call base class handler !!
			}
			*/
			if (Fl::visible_focus()) handle(FL_FOCUS);
			return Fl_Value_Slider::handle(event);
		case FL_RELEASE:
			return 1; // Don't call base class handler !!
			// It would prevent double click from working
		case FL_ENTER: // Needed for tooltip
			// printf("FL_ENTER\n");
			return 1;
		case FL_LEAVE: // Needed for tooltip
			// printf("FL_LEAVE\n");
			return 1;
		case FL_MOUSEWHEEL:
			// printf("FL_MOUSEWHEEL %d\n",Fl::event_dy());
			val-=step*Fl::event_dy();
			if (val<maxval){
				val+=step;
				if (val>maxval){
					val=maxval;
				}
			}
			if (val>minval){
				val-=step;
				if (val<minval){
					val=minval;
				}
			}
			this->value(val);
			this->callback()((Fl_Widget*)this, 0);
			return 1;
	}
	return Fl_Value_Slider::handle(event);
}

// ALSA midi
snd_seq_t *seq_handle;
int portid;
int npfd;
struct pollfd *pfd;
int pitchbend_in=0, last_note=-1;

snd_seq_t *open_seq(const char *midiName, int &portid) {
  snd_seq_t *seq_handle;
  if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
	fprintf(stderr, "Error opening ALSA sequencer.\n");
	exit(1);
  }
  snd_seq_set_client_name(seq_handle, midiName);
  if ((portid = snd_seq_create_simple_port(seq_handle, midiName,
			SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ|
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
	fprintf(stderr, "Error creating sequencer port.\n");
	exit(1);
  }
  return(seq_handle);
}

void send_pitchbend(int pb){
	snd_seq_event_t ev;
	snd_seq_ev_set_source( &ev, portid );
	snd_seq_ev_set_subs( &ev );
	snd_seq_ev_set_direct( &ev );
	ev.type = SND_SEQ_EVENT_PITCHBEND;
	// pitchbend_out=CLAMP(pitchbend_in+pitchbend_offset, -8192, 8191);
	ev.data.control.value = pb; // pitchbend_out;
	snd_seq_event_output_direct( seq_handle, &ev );
}

char note_names[][3]={
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
int note_offset[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
Note_Slider * note_sliders[12];

static void parm_callback(Fl_Widget* o, void*) {
	int note, offset, pitchbend_out;
	if (o){
		note=o->argument();
		offset=((Fl_Valuator*)o)->value();
		note_offset[note]=offset;
		// Todo if current note send new pitch bend value ??
		if(note==last_note){
			pitchbend_out=CLAMP(pitchbend_in+offset, -8192, 8191);
			send_pitchbend(pitchbend_out);
		}
	}
}

int load_file(const char *filename)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
	const char *ptr;
	char *ptr2;

	int note, offset, errs=0;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return(-1);

	printf("Opened %s\n", filename);

    while ((read = getline(&line, &len, fp)) != -1) {
        // printf("Retrieved line of length %zu:\n", read);
        // printf("%s", line);
		note=strtonote(line, &ptr);
        offset=strtol(ptr, &ptr2, 10);
        // printf("%u %u\n", note, offset);
        if (*ptr2 && *ptr2 != '\n') show_err("Extra ignored: %s", ptr2);
        if (ptr2==ptr){
			show_err("Missing offset after %s", line); errs++;
		}else if(offset<-8192 or offset >8191){
			show_err("Illegal offset %d", offset); errs++;
		}else{
			note_offset[note]=offset;
			note_sliders[note]->value(offset);
			note_sliders[note]->redraw();
		}
    }

    fclose(fp);
    
	printf("Closed %s\n", filename);
	
    if (line)
        free(line);
    return(errs!=0);
}

void write_file(const char *filename, bool auto_ext ){
	char *filename2, *filename3;
	// char line[10];
	FILE *file;
	filename2=(char *)malloc(strlen(filename)+6);
	if (filename2){
		strcpy(filename2, filename);
		if (auto_ext && strchr(filename2,'.') == 0)
			strcat(filename2, ".conf");
		// Check existence and prompt before overwriting
		if( access( filename2, F_OK ) != -1 ) {
			if(fl_choice("File already exists. Overwrite?","Yes", "No", 0)){
				free(filename2);
				fprintf(stderr, "Write file cancelled.\n");
				return;
			}
			printf("Overwriting %s\n", filename2);
		}
		file=fopen(filename2,"wb");
		if (file==0){
			fprintf(stderr,"ERROR: Can not open file %s\n", filename2);
		}else{		
			for (int note=0; note<12;note++){
				fprintf(file, "%s %d\n", note_names[note], note_offset[note]);
			}
			fclose(file);
		}
		free(filename2);
	}else{
		fprintf(stderr, "ERROR: Cannot allocate memory");
	}
}



// GUI creation helper function and callbacks

void fc_load_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	// ?? should remember directory across calls
	if(w->visible()) return; // Do nothing until user has pressed ok
	int errs=load_file(w->value());
    for(int note=0; note<12; note++) note_sliders[note]->redraw();
	if(errs) fl_alert("Error(s) loading file %s", w->value());
}

static void btn_load_callback(Fl_Widget* o, void*) {
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","config files(*.conf)",Fl_File_Chooser::SINGLE,"Input file");
    fc->preview(0);
    fc->callback(fc_load_callback);
    fc->show();
}

void fc_save_callback(Fl_File_Chooser *w, void *userdata){
	// ?? should remember directory across calls
	if(w->visible()) return; // Do nothing until user has pressed ok
	write_file(w->value(), w->filter_value() == 0);
}

static void btn_save_callback(Fl_Widget* o, void*) {
   Fl_File_Chooser *fc = new Fl_File_Chooser(".","config files(*.conf)",Fl_File_Chooser::CREATE,"Output file");
    // fc->value(tone_name);
    fc->preview(0);
    fc->callback(fc_save_callback);
    fc->show();
}


Fl_Value_Slider *make_value_slider(int x, int y, int w, int h, const char * label, int note_number){
	// Fl_Value_Slider *o = new Fl_Value_Slider(x, y, w, h, label);
	Note_Slider *o = new Note_Slider(x, y, w, h, label);
	o->argument(note_number);
	o->bounds(-8192, +8191); // Possibly useless
	o->range(+8191, -8192); // low to high!
	o->step(1);
	o->align(FL_ALIGN_TOP);
	o->callback((Fl_Callback*)parm_callback); // Handles both tone and patch parameters
	o->slider_size(.04);
	o->textsize(11);
	o->tooltip("Use mouse wheel or keyboard arrows for fine adjustment");
	note_sliders[note_number]=o;
	return(o);
}

// GUI creation
Fl_Double_Window* make_window() {
	int w=40, h=25, spacing=5;
	int x=spacing, y=spacing;
    int ww=spacing+13*(w+spacing);
    int hh=12*h+4*spacing;
	main_window = new Fl_Double_Window(ww, hh, "Autobend");
	y+=h+spacing; // room for labels
	make_value_slider(x, y, w, 11*h, "C", 0); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "C#", 1); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "D", 2); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "D#", 3); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "E", 4); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "F", 5); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "F#", 6); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "G", 7); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "G#", 8); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "A", 9); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "A#", 10); x += w + spacing;
	make_value_slider(x, y, w, 11*h, "B", 11); x += w + spacing;
	Fl_Button *b=new Fl_Button(x, y, w, h, "Load"); y += h+spacing;
	b->callback((Fl_Callback*)btn_load_callback);
	b=new Fl_Button(x, y, w, h, "Save"); y += h+spacing;
	b->callback((Fl_Callback*)btn_save_callback);
	main_window->end();
	main_window->resizable(main_window);
	return main_window;
}

///////////////////////////
// MIDI receive callback //
///////////////////////////

static void *alsa_midi_process(void *) {
	snd_seq_event_t *ev;
	snd_seq_event_t ev2;
	int pitchbend_offset=0, pitchbend_out;
	do {
		while (snd_seq_event_input(seq_handle, &ev)) {
			switch (ev->type) {
			// ?? do something about noteoff (play highest remaining note?)
			// what about hold ?
			case SND_SEQ_EVENT_PITCHBEND:
				pitchbend_in=ev->data.control.value;
				pitchbend_out=CLAMP(pitchbend_in+pitchbend_offset, -8192, 8191);
				ev->data.control.value=pitchbend_out;
				snd_seq_ev_set_source( ev, portid );
				snd_seq_ev_set_subs( ev );
				snd_seq_ev_set_direct( ev );
				snd_seq_event_output_direct( seq_handle, ev );
				snd_seq_free_event(ev);
			break;
			case SND_SEQ_EVENT_NOTEON:
				last_note=ev->data.note.note % 12;
				pitchbend_offset=note_offset[last_note];
				// send pitchbend out
				snd_seq_ev_set_source( &ev2, portid );
				snd_seq_ev_set_subs( &ev2 );
				snd_seq_ev_set_direct( &ev2 );
				ev2.type = SND_SEQ_EVENT_PITCHBEND;
				pitchbend_out=CLAMP(pitchbend_in+pitchbend_offset, -8192, 8191);
				ev2.data.control.value = pitchbend_out;
				snd_seq_event_output_direct( seq_handle, &ev2 );
				// Fall through default (send note)
			default:
				// Else resend what we received
				// see https://tldp.org/HOWTO/MIDI-HOWTO-9.html
				snd_seq_ev_set_source( ev, portid );
				snd_seq_ev_set_subs( ev );
				snd_seq_ev_set_direct( ev );
				snd_seq_event_output_direct( seq_handle, ev );
				snd_seq_free_event(ev);
			}
		} // end of first while, emptying the seqdata queue
	} while (true); // doing forever, was  (snd_seq_event_input_pending(seq_handle, 0) > 0);
	return 0;
}


int main(int argc, char **argv) {
	
	Fl_Window* win = make_window();
//	win->callback(window_callback);

	for(int i=1; i<argc;i++)
		load_file(argv[i]);


	// midi init
	pthread_t midithread;
	seq_handle = open_seq("AutoBend", portid);
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

	// create the thread and tell it to use Midi::work as thread function
	int err = pthread_create(&midithread, NULL, alsa_midi_process, seq_handle);
	if (err) {
		show_err("Error %u creating MIDI thread\n", err);
		// should exit? This is non-blocking for one-way use from editor to MKS-50.
	}

	// FLTK main loop
	win->show();
	Fl::run();
	// wait for the midi thread to shutdown
	pthread_cancel(midithread);
	// release Alsa Midi connection
	snd_seq_close(seq_handle);
	
	printf("Bye!\n");
}
