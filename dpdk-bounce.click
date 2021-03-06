/**
 * This simple configuration bounces packets from a single interface to itself.
 * MAC addresses are inverted
 *
 * A minimal launch line would be :
 * sudo bin/click --dpdk -- conf/dpdk/dpdk-bounce.click
 */

define ($print false)

FromDPDKDevice(0, NDESC 256)
	-> EtherMirror()
        -> EtherMirror()
        -> EtherMirror()
//        -> EtherRewrite(b8:ce:f6:31:3b:57, aa:e2:c9:59:3c:02)
	-> Print(ACTIVE $print)
//        -> RandomBitErrors(1)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
        -> MoveData(15, 15, 50)
//        -> RandomBitErrors(1)
	-> ToDPDKDevice(0, NDESC 256)
