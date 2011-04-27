/*

  D-Bus messaging interface

  (c) 2008 jpv (jpv950@gmail.com)

  (c) 2008 Jindrich Makovicka (makovick@gmail.com)

  This file is part of pgl.

  pgl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  pgl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GNU Emacs; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "dbus.h"

static DBusConnection *dbconn = NULL;

int pgl_dbus_init() {

    DBusError dberr;
    int req;

    dbus_error_init (&dberr);
    dbconn = dbus_bus_get (DBUS_BUS_SYSTEM, &dberr);
    if (dbus_error_is_set (&dberr)) {
        fprintf(stderr, "Connection Error (%s)\n", dberr.message);
        dbus_error_free(&dberr);
    }
    if (dbconn == NULL) {
      return -1;
    }
    //FIXME: It will be difficult to use do_log() in dbus.c because
    //            a) Of compiler errors(undefined symbols etc
    //            b) do_log() calls pgl_dbus_send() 
    //       Is fprintf(stderr,...) ok? (jimtb)
    //do_log(LOG_INFO, "INFO: Connected to system bus.") 
    /* FIXME: need d-bus policy privileges for this to work, pgld has to get them! */
//     dbus_error_init (&dberr); /* FIXME: Why commented out? */
    req = dbus_bus_request_name (dbconn, PGL_DBUS_PUBLIC_NAME,
                                 DBUS_NAME_FLAG_DO_NOT_QUEUE, &dberr); /* TODO: DBUS_NAME_FLAG_ALLOW_REPLACEMENT goes here */
    if (dbus_error_is_set (&dberr)) {
//        do_log(LOG_ERR, "ERROR: requesting name: %s.", dberr.message); 
	fprintf(stderr, "ERROR: requesting name %s.\n", dberr.message);
        dbus_error_free(&dberr);
        return -1;
    }
    if (req == DBUS_REQUEST_NAME_REPLY_EXISTS) {
        /*TODO: req = dbus_bus_request_name (dbconn, NFB_DBUS_PUBLIC_NAME,
                                 DBUS_NAME_FLAG_REPLACE_EXISTING, &dberr); */
//        do_log(LOG_WARNING, "WARN: pgld is already running."); 
	fprintf(stderr, "WARN: pgld is already running.\n");
        return -1;
    }
    return 0;
}

void pgl_dbus_send(const char *format, va_list ap) {
	
    if (dbconn == NULL) {
	fprintf( stderr, "ERROR: dbus_send() called with NULL connection!\n");
	exit(1);
    }


    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage *dbmsg = NULL;
    DBusMessageIter dbiter;
    char *msgPtr;
    char msg[MSG_SIZE];
    vsnprintf(msg, sizeof msg, format, ap);
    msgPtr = msg;
    /* create dbus signal */
    dbmsg = dbus_message_new_signal ("/org/netfilter/pgl",
                                         "org.netfilter.pgl",
                                         "pgld_message");
    if (dbmsg == NULL) {
        fprintf(stderr, "ERROR: NULL dbus message!\n");
	exit(1);	
    }

    dbus_message_iter_init_append(dbmsg, &dbiter);
    if (!dbus_message_iter_append_basic(&dbiter, DBUS_TYPE_STRING, &msgPtr)) { //The API needs a double pointer, otherwise causes seg. fault 
      fprintf(stderr, "Out Of Memory!\n");
    }
    if (!dbus_connection_send (dbconn, dbmsg, &serial)) {
        fprintf(stderr, "Out Of Memory!\n");
    }
    dbus_connection_flush(dbconn);
    dbus_message_unref(dbmsg);
    printf( "DEBUG: DBUS_SEND() SUCESSFUL!\n" );

}
