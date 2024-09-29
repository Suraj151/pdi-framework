/* stack initialize the services in setup and
 * should serve in loop continueously
 */

#include <PdiStack.h>

void setup() {
 PdiStack.initialize();
}

void loop() {
 PdiStack.serve();
}
