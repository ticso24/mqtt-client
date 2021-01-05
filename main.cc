/*
 * Copyright (c) 2020 Bernd Walter Computer Technology
 * http://www.bwct.de
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define LICHTSCHALTER_KELLERTREPPE_OBEN1 "cicely/mb1.cicely.de/255/input0"
#define LICHTSCHALTER_KELLERTREPPE_OBEN2 "cicely/mb1.cicely.de/255/input1"
#define LICHT_KELLERTREPPE "cicely/mb1.cicely.de/255/relais0"
#define LICHT_KELLERGANG "cicely/mb1.cicely.de/255/relais1"

#include "main.h"
#include <mosquitto.h>
#include "mqtt.h"

a_refptr<JSON> config;

void
siginit()
{
	struct sigaction sa;

	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);
}

void
sighandler(int sig)
{

	switch (sig) {
		case SIGPIPE:
		break;
		default:
		break;
	}
}

static MQTT mqtt;

void*
ProcessLoop(void* arg)
{
	mqtt.subscribe("cicely/#");
	for(;;) {
		try {
			static bool init;
			static bool schalter1_alt;
			static bool schalter2_alt;

			bool schalter1 = mqtt[LICHTSCHALTER_KELLERTREPPE_OBEN1] == "1";
			bool schalter2 = mqtt[LICHTSCHALTER_KELLERTREPPE_OBEN2] == "1";
			bool licht1 = false;
			bool licht2 = false;
			try {
				licht1 = mqtt[LICHT_KELLERTREPPE] == "1";
				licht2 = mqtt[LICHT_KELLERGANG] == "1";
			} catch(...) {
			}

			if (init) {
				if (schalter1 != schalter1_alt || schalter2 != schalter2_alt) {
					if (licht1) {
						mqtt.publish_ifchanged(LICHT_KELLERTREPPE, "0");
						mqtt.publish_ifchanged(LICHT_KELLERGANG, "0");
					} else {
						mqtt.publish_ifchanged(LICHT_KELLERTREPPE, "1");
						mqtt.publish_ifchanged(LICHT_KELLERGANG, "1");
					}
				}
			}

			init = true;
			schalter1_alt = schalter1;
			schalter2_alt = schalter2;

			usleep(100000);
		} catch(...) {
		}
	}
}

int
main(int argc, char *argv[]) {
	String configfile = "/usr/local/etc/mqtt-client.conf";

	int ch;
	bool debug = false;

	while ((ch = getopt(argc, argv, "dc:")) != -1) {
		switch (ch) {
		case 'c':
			configfile = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case '?':
			default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!debug) {
		daemon(0, 0);
	}

	{
		File f;
		f.open(configfile, O_RDONLY);
		String json(f);
		config = new(JSON);
		config->parse(json);
	}

	mosquitto_lib_init();

	a_refptr<JSON> my_config = config;
	JSON& cfg = *my_config.get();

	if (cfg.exists("mqtt")) {
		JSON& mqtt_cfg = cfg["mqtt"];
		String id = mqtt_cfg["id"];
		mqtt.id = id;
		String host = mqtt_cfg["host"];
		mqtt.host = host;
		String port = mqtt_cfg["port"];
		mqtt.port = port.getll();
		String username = mqtt_cfg["username"];
		mqtt.username = username;
		String password = mqtt_cfg["password"];
		mqtt.password = password;
		String maintopic = mqtt_cfg["maintopic"];
		mqtt.maintopic = maintopic;
		mqtt.connect();
		String willtopic = maintopic + "/status";
		mqtt.publish(willtopic, "online", true);
		mqtt.publish(maintopic + "/product", "mb_client", true);
		mqtt.publish(maintopic + "/version", "0.2", true);
	} else {
		printf("no mqtt setup in config\n");
		exit(1);
	}

	// start process loop
	{
		pthread_t process_thread;
		pthread_create(&process_thread, NULL, ProcessLoop, NULL);
		pthread_detach(process_thread);
	}

	for (;;) {
		sleep(10);
	}
	return 0;
}

void
usage(void) {

	printf("usage: mqtt_client [-d] [-c configfile]\n");
	exit(1);
}

