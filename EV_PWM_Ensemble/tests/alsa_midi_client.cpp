#include "alsa_midi_client.h"
#include <dlfcn.h>
#include <iostream>
#include <cstring>

// ALSA Constants (from asoundlib.h)
#define SND_SEQ_OPEN_OUTPUT 1
#define SND_SEQ_PORT_CAP_READ (1<<10)
#define SND_SEQ_PORT_CAP_SUBS_READ (1<<11)
#define SND_SEQ_PORT_TYPE_MIDI_GENERIC (1<<21)
#define SND_SEQ_EVENT_NOTEON 6
#define SND_SEQ_EVENT_NOTEOFF 7
#define SND_SEQ_TIME_STAMP_TICK 0
#define SND_SEQ_TIME_MODE_ABS 0

typedef struct _snd_seq snd_seq_t;
typedef struct snd_seq_addr {
	unsigned char client;
	unsigned char port;
} snd_seq_addr_t;

typedef struct snd_seq_tick_time {
    unsigned int tick;
} snd_seq_tick_time_t;

typedef struct snd_seq_real_time {
	unsigned int tv_sec;
	unsigned int tv_nsec;
} snd_seq_real_time_t;

typedef union snd_seq_timestamp {
	snd_seq_tick_time_t tick;
	snd_seq_real_time_t time;
} snd_seq_timestamp_t;

typedef struct snd_seq_event {
	unsigned char type;
	unsigned char flags;
	unsigned char tag;
	unsigned char queue;
	snd_seq_timestamp_t time;
	snd_seq_addr_t source;
	snd_seq_addr_t dest;
	union {
		struct {
			unsigned char channel;
			unsigned char note;
			unsigned char velocity;
			unsigned char off_velocity;
			unsigned int duration;
		} note;
		struct {
			unsigned char channel;
			unsigned char param;
			signed int value;
		} control;
		struct {
			unsigned int len;
			void *ptr;
		} ext;
        unsigned char raw8[8];
	} data;
} snd_seq_event_t;

// Function pointers for ALSA functions
typedef int (*snd_seq_open_t)(snd_seq_t **, const char *, int, int);
typedef int (*snd_seq_close_t)(snd_seq_t *);
typedef int (*snd_seq_set_client_name_t)(snd_seq_t *, const char *);
typedef int (*snd_seq_create_simple_port_t)(snd_seq_t *, const char *, unsigned int, unsigned int);
typedef int (*snd_seq_connect_to_t)(snd_seq_t *, int, int, int);
typedef int (*snd_seq_event_output_t)(snd_seq_t *, snd_seq_event_t *);
typedef int (*snd_seq_drain_output_t)(snd_seq_t *);

static snd_seq_open_t p_snd_seq_open;
static snd_seq_close_t p_snd_seq_close;
static snd_seq_set_client_name_t p_snd_seq_set_client_name;
static snd_seq_create_simple_port_t p_snd_seq_create_simple_port;
static snd_seq_connect_to_t p_snd_seq_connect_to;
static snd_seq_event_output_t p_snd_seq_event_output;
static snd_seq_drain_output_t p_snd_seq_drain_output;

ALSAMIDIClient::ALSAMIDIClient() : handle(nullptr), seq(nullptr), port(-1) {}

ALSAMIDIClient::~ALSAMIDIClient() {
    close();
}

bool ALSAMIDIClient::open(const std::string& clientName) {
    handle = dlopen("libasound.so.2", RTLD_LAZY);
    if (!handle) {
        return false;
    }

    p_snd_seq_open = (snd_seq_open_t)dlsym(handle, "snd_seq_open");
    p_snd_seq_close = (snd_seq_close_t)dlsym(handle, "snd_seq_close");
    p_snd_seq_set_client_name = (snd_seq_set_client_name_t)dlsym(handle, "snd_seq_set_client_name");
    p_snd_seq_create_simple_port = (snd_seq_create_simple_port_t)dlsym(handle, "snd_seq_create_simple_port");
    p_snd_seq_connect_to = (snd_seq_connect_to_t)dlsym(handle, "snd_seq_connect_to");
    p_snd_seq_event_output = (snd_seq_event_output_t)dlsym(handle, "snd_seq_event_output");
    p_snd_seq_drain_output = (snd_seq_drain_output_t)dlsym(handle, "snd_seq_drain_output");

    if (!p_snd_seq_open || !p_snd_seq_close || !p_snd_seq_set_client_name ||
        !p_snd_seq_create_simple_port || !p_snd_seq_connect_to || !p_snd_seq_event_output || !p_snd_seq_drain_output) {
        return false;
    }

    int ret = p_snd_seq_open((snd_seq_t**)&seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
    if (ret < 0) {
        return false;
    }

    p_snd_seq_set_client_name((snd_seq_t*)seq, clientName.c_str());
    port = p_snd_seq_create_simple_port((snd_seq_t*)seq, "Output Port",
                                        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    return true;
}

void ALSAMIDIClient::close() {
    if (seq && p_snd_seq_close) {
        p_snd_seq_close((snd_seq_t*)seq);
        seq = nullptr;
    }
    if (handle) {
        dlclose(handle);
        handle = nullptr;
    }
}

bool ALSAMIDIClient::connect(int destClient, int destPort) {
    if (!seq) return false;
    int ret = p_snd_seq_connect_to((snd_seq_t*)seq, port, destClient, destPort);
    // 16 is EEXIST, which means already connected.
    if (ret < 0 && ret != -16) {
        std::cerr << "ALSA Connect failed: " << ret << " (dest " << destClient << ":" << destPort << ")" << std::endl;
        return false;
    }
    return true;
}

void ALSAMIDIClient::sendNoteOn(int channel, int note, int velocity) {
    if (!seq) return;
    snd_seq_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SND_SEQ_EVENT_NOTEON;
    ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
    ev.source.port = (unsigned char)port;
    ev.dest.client = 254; // SND_SEQ_ADDRESS_SUBSCRIBERS
    ev.dest.port = 254;
    ev.queue = 253; // SND_SEQ_QUEUE_DIRECT
    ev.data.note.channel = (unsigned char)channel;
    ev.data.note.note = (unsigned char)note;
    ev.data.note.velocity = (unsigned char)velocity;
    p_snd_seq_event_output((snd_seq_t*)seq, &ev);
    p_snd_seq_drain_output((snd_seq_t*)seq);
}

void ALSAMIDIClient::sendNoteOff(int channel, int note, int velocity) {
    if (!seq) return;
    snd_seq_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SND_SEQ_EVENT_NOTEOFF;
    ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
    ev.source.port = (unsigned char)port;
    ev.dest.client = 254;
    ev.dest.port = 254;
    ev.queue = 253;
    ev.data.note.channel = (unsigned char)channel;
    ev.data.note.note = (unsigned char)note;
    ev.data.note.velocity = (unsigned char)velocity;
    p_snd_seq_event_output((snd_seq_t*)seq, &ev);
    p_snd_seq_drain_output((snd_seq_t*)seq);
}

std::vector<ALSAMIDIClient::PortInfo> ALSAMIDIClient::listPorts() {
    return {};
}
