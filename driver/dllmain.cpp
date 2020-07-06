/*
**
** Copyright (C) 2020 Ashcon Mohseninia
** Author: Ashcon Mohseninia <ashcon50@gmail.com>
**
** This library is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, <http://www.gnu.org/licenses/>.
**
*/

#include "pch.h"
#include "Logger.h"
#include "commserver.h"
#include "macchina-passthru.h"

bool startup() {
    LOGGER.writeToFile("\n\n##RESTART##\n");
    return commserver::CreateCommThread();
}

void close() {
    commserver::CloseCommThread();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        LOGGER.logDebug("APIENTRY", "Process attached");
        if (!startup()) {
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        LOGGER.logDebug("APIENTRY", "Process detached");
        close();
        break;
    }
    return TRUE;
}

