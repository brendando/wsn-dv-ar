/**
 * \file
 *         Template Contiki C file
 * \author
 *         Brendan Do
 */

#include "contiki.h"

/*---------------------------------------------------------------------------*/
PROCESS(generic_process, "Generic process");
AUTOSTART_PROCESSES(&generic_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(generic_process, ev, data) {

  	PROCESS_BEGIN();

	while (1) {
		
		clock_wait(CLOCK_SECOND);
	}

  	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
