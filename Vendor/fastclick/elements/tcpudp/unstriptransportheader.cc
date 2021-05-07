/*
 * UnstripTransportHeader.{cc,hh} -- put IP header back based on annotation
 * Benjie Chen
 *
 * Computational batching support
 * by Georgios Katsikas
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2016 KTH Royal Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "unstriptransportheader.hh"
#include "striptransportheader.hh"
#include <clicknet/ip.h>
CLICK_DECLS

UnstripTransportHeader::UnstripTransportHeader()
{
}

UnstripTransportHeader::~UnstripTransportHeader()
{
}

Packet *
UnstripTransportHeader::simple_action(Packet *p)
{
    assert(p->transport_header());
    ptrdiff_t offset = p->transport_header() - p->data();
    if (offset < 0)
        p = p->push(-offset);	// should never create a new packet
    return p;
}

#if HAVE_BATCH
PacketBatch*
UnstripTransportHeader::simple_action_batch(PacketBatch *batch)
{
    EXECUTE_FOR_EACH_PACKET(UnstripTransportHeader::simple_action, batch);
    return batch;
}
#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(UnstripTransportHeader)
ELEMENT_MT_SAFE(UnstripTransportHeader)
