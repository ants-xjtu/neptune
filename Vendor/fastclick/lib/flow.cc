// -*- c-basic-offset: 4; related-file-name: "../include/click/flow/flow.hh" -*-
/*
 * flow.{cc,hh} -- the Flow class
 * Tom Barbette
 *
 * Copyright (c) 2015 University of Liege
 * Copyright (c) 2019 KTH Royal Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */


#include <click/config.h>
#include <click/glue.hh>
#include <stdlib.h>
#include <regex>
#include <click/flow/flow.hh>

CLICK_DECLS

#ifdef HAVE_FLOW
__thread FlowControlBlock* fcb_stack = 0;
__thread FlowTableHolder* fcb_table = 0;
#endif

CLICK_ENDDECLS
