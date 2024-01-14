#include "sectohms.h"

void secToHMS(long interval, char * buf) {
    int hours = interval / 3600;
    int minutes = (interval % 3600) / 60;
    int seconds = (interval % 60);
    sprintf(buf, "%3dh%02dm%02ds",hours, minutes, seconds);
    return;
}
