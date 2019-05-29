#include <nsbase.h>

CNSMap< int, CNSTimer::CTimerObject >	CNSTimer::sTimer;
CNSSet< int >							CNSTimer::sRemoveList;
int CNSTimer::timerID;

