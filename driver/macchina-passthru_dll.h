#pragma once
#include "j2534_v0404.h"

extern PTOPEN	PassThruOpen;
extern PTCLOSE	PassThruClose;
extern PTCONNECT PassThruConnect;
extern PTDISCONNECT PassThruDisconnect;
extern PTREADMSGS PassThruReadMsgs;
extern PTWRITEMSGS PassThruWriteMsgs;
extern PTSTARTPERIODICMSG PassThruStartPeriodicMsg;
extern PTSTOPPERIODICMSG PassThruStopPeriodicMsg;
extern PTSTARTMSGFILTER PassThruStartMsgFilter;
extern PTSTOPMSGFILTER PassThruStopMsgFilter;
extern PTSETPROGRAMMINGVOLTAGE PassThruSetProgrammingVoltage;
extern PTREADVERSION PassThruReadVersion;
extern PTGETLASTERROR PassThruGetLastError;
extern PTIOCTL PassThruIoctl;
