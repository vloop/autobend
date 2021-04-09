 /*
 * This file is part of autohold, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// Build with: 
// gcc autohold.cxx -o autohold -lasound -lpthread -lstdc++

// TODO
// sigint handler
// command line options:
//   channel
// autoconnect
 
#include <alsa/asoundlib.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

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
    va_end(args);
}

int verbose=0;

// ALSA midi
snd_seq_t *seq_handle;
int portid;
int npfd;
struct pollfd *pfd;
// int pitchbend_in=0, last_note=-1;
bool hold_active, sost_active, soft_active;
// int pitchbend_offset=0;
int midi_channel=0; // 0..15 for channels 1..16

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

char note_names[][3]={
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

bool key_down[128], key_damp[128], key_sost[128];
int key_stack[128];
int *key_stack_ptr=key_stack;
bool sostenuto=false, unacorda=false;
float unacorda_factor=0.5;

///////////////////////////
// MIDI receive callback //
///////////////////////////

static void do_all_notes_off(){
	for(int i=0; i<128;i++){
		key_down[i]=false;
		key_damp[i]=false;
		key_sost[i]=false;
	}
	// hold_active=false; // ?? unsure, need to check actual behaviour
	key_stack_ptr=key_stack;
}

static void remove_note_from_stack(int note){
	// Removes all occurences of note
	// even though there should be only one
	int *src, *dest;
	src=key_stack;
	dest=key_stack;
	while(src<key_stack_ptr){
		if(*src==note){
			src++;
		}else{
			*dest++=*src++;
		}
	}
	key_stack_ptr=dest;
}

static void add_note_to_stack(int note){
	*key_stack_ptr++=note;
}

void send_noteoff(int note, int channel){
	snd_seq_event_t ev;
	snd_seq_ev_set_source( &ev, portid );
	snd_seq_ev_set_subs( &ev );
	snd_seq_ev_set_direct( &ev );
	ev.type = SND_SEQ_EVENT_NOTEOFF;
	ev.data.note.note = note;
	ev.data.note.channel=channel;
	snd_seq_event_output_direct( seq_handle, &ev );
}

static void *alsa_midi_process(void *) {
	snd_seq_event_t *ev;
	snd_seq_event_t ev2;
	int note, vel;
	bool do_resend;
	while(true) {
		while (snd_seq_event_input(seq_handle, &ev)) {
			do_resend=true; // default to passthru
			switch (ev->type) {
			case SND_SEQ_EVENT_NOTEON:
				if (ev->data.note.channel==midi_channel){
					note = ev->data.note.note;
					// printf("noteon %d\n", last_note);
					key_down[note] = true;
					if(!sostenuto) key_damp[note] = hold_active;
					// key_sost stays as it is !!
					add_note_to_stack(note);
					if(unacorda && soft_active){
						vel = unacorda_factor * ev->data.note.velocity;
						ev->data.note.velocity=vel>127?127:vel;
					}
				}
			break;
			case SND_SEQ_EVENT_CONTROLLER:
				if (ev->data.control.channel==midi_channel){
					if(ev->data.control.param>=123 || ev->data.control.param==120){
						// All notes off and similar
						do_all_notes_off();
					}else if (ev->data.control.param==64){ // CC#64 (Sustain)
						// Hold
						do_resend=false;
						hold_active=ev->data.control.value>63;
						if(hold_active) {
							for(int i=0; i<128;i++) key_damp[i]=key_down[i];
						}else{ // Hold is released, turn off held notes
							for(int i=0; i<128;i++){
								key_damp[i]=false;
								if(!key_down[i] && !key_sost[i]){
									remove_note_from_stack(i);
									// Need to send note off for previously held notes
									send_noteoff(i, midi_channel);
								}
							}
						}
					}else if (ev->data.control.param==66){ // CC#66 (Sostenuto)
						do_resend=false;
						sost_active=ev->data.control.value>63;
						if(sost_active) {
							for(int i=0; i<128;i++) key_sost[i]=key_down[i];
						}else{ // Sostenuto is released, turn off notes as needed
							for(int i=0; i<128;i++){
								key_sost[i]=false;
								if(!key_down[i] && !key_damp[i]){
									remove_note_from_stack(i);
									// Need to send note off for previously held notes
									send_noteoff(i, midi_channel);
								}
							}
						}
					}else if (ev->data.control.param==67){ // CC#67 (Una corda)
						// We could remap velocity down if active
						do_resend=false;
						soft_active=ev->data.control.value>63;
					}
				}
			break;
			case SND_SEQ_EVENT_NOTEOFF:
				if (ev->data.note.channel==midi_channel){
					note = ev->data.note.note;
					key_down[note]=false;
					if(key_damp[note] || key_sost[note]){
						do_resend=false;
					}else{
						// printf("noteoff %d\n", note);
						remove_note_from_stack(note);
					}
				}
			break;
			default:
				// printf("type %d\n", ev->type);
			break;
			} // End of switch
			// Resend what we received
			// see https://tldp.org/HOWTO/MIDI-HOWTO-9.html
			// TODO Don't resend note off if hold is active and note is held
			if (do_resend){
				snd_seq_ev_set_source( ev, portid );
				snd_seq_ev_set_subs( ev );
				snd_seq_ev_set_direct( ev );
				snd_seq_event_output_direct( seq_handle, ev );
				snd_seq_free_event(ev);
			}
		} // end of first while, emptying the seqdata queue
	}
	return 0;
}


void Usage(char *programName)
{
	fprintf(stderr,"%s usage:\n",programName);
	fprintf(stderr,"%s [option...]\n",programName);
	fprintf(stderr,"  -v      verbose\n");
	fprintf(stderr,"  -r      replace damper with sostenuto\n");
	fprintf(stderr,"  -s [nn] simulate soft pedal, reduce velocity to nn percent\n");
	fprintf(stderr,"  -c nn   use midi channel nn 1..16\n");
}

void optionError(char *programName, char *option)
{
	fprintf(stderr,"ERROR: unknown option \"%s\"\n",option);
	Usage(programName);
}

void valueError(char *programName, char *option)
{
	fprintf(stderr,"ERROR: illegal value \"%s\"\n",option);
	Usage(programName);
}

int HandleOptions(int argc,char *argv[])
{
	int i,firstnonoption=0;
	char *endptr;

	for (i=1; i< argc;i++) {
		if (argv[i][0] == '/' || argv[i][0] == '-') {
			// fprintf(stderr,"Option %i: %c %s\n",i,argv[i][1],argv[i]+1);
			switch (argv[i][1]) {
				/* An argument -? means help is requested */
				case '?':
					Usage(argv[0]);
					break;
				case 'h':
				case 'H':
					if (!strncmp(argv[i]+1,"help",5)) { // linux - no strnicmp
						Usage(argv[0]);
						break;
					}
					optionError(argv[0], argv[i]);
					break;
				case 'r':
				case 'R':
					sostenuto=true;
					break;
				case 's':
				case 'S':
					unacorda=true;
					if (i+1<argc && argv[i+1][0]!='-'){
						unacorda_factor=strtoul(argv[++i], &endptr, 10)/100.0;
						if (*endptr) valueError(argv[0], argv[i]);
					}else{ // If no value given, not an error, use default
						unacorda_factor=0.5;
					}
					break;
				case 'c':
				case 'C':
					unacorda=true;
					if (i+1<argc){
						midi_channel=strtoul(argv[++i], &endptr, 10)-1;
						if (*endptr || midi_channel>15 || midi_channel<0 ) valueError(argv[0], argv[i]);
					}else{
						valueError(argv[0], argv[i]);
					}
					break;
				case 'v':
				case 'V':
					verbose++;
					break;
				default:
					optionError(argv[0], argv[i]);
					break;
			}
		}
		else {
			firstnonoption = i;
			break;
		}
	}
	return firstnonoption;
}


int main(int argc, char **argv) {

	HandleOptions(argc, argv);

	do_all_notes_off();

	// midi init
	pthread_t midithread;
	seq_handle = open_seq("AutoHold", portid);
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

	// create the thread
	int err = pthread_create(&midithread, NULL, alsa_midi_process, seq_handle);
	if (err) {
		show_err("Error %u creating MIDI thread\n", err);
		exit(-1);
	}

	printf("Handling pedals on channel %u\n", midi_channel);
	if (verbose>0 && unacorda) printf("Handling unacorda, velocity factor %f\n", unacorda_factor);
	printf("Press enter to exit");
	getchar();

	// wait for the midi thread to shutdown
	pthread_cancel(midithread);
	// release Alsa Midi connection
	snd_seq_close(seq_handle);
	
	printf("Bye!\n");
}
