/*
** This file is a part of PRADS.
**
** Copyright (C) 2009, Redpill Linpro
** Copyright (C) 2009, Edward Fjellskål <edward.fjellskaal@redpill-linpro.com>
** Copyright (C) 2011, Kacper Wysocki <kacper.wysocki@redpill-linpro.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
*/
/*  P R O T O T Y P E S  ******************************************************/
output_plugin *init_log_file();
int init_output_log_file (output_plugin *p, const char *c, int flags);
void read_report_file (output_plugin *p);
int parse_raw_report (bstring line);
void file_os(output_plugin*,asset *main, os_asset *os, connection*);
void file_service(output_plugin*,asset *main, serv_asset *service, connection*);
void file_arp(output_plugin*, asset *main);
int end_output_log_file (output_plugin*);

