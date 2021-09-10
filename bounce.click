/**
 * This simple configuration bounces packets from a single interface to itself.
 * MAC addresses are inverted
 *
 * A minimal launch line would be :
 * sudo bin/click --dpdk -- conf/dpdk/dpdk-bounce.click
 */

define ($print false)

FromDevice(ens3f0, PROMISC true)
	-> EtherMirror()
        -> Queue
        -> EtherMirror()
        -> EtherMirror()
//        -> EtherRewrite(b8:ce:f6:31:3b:57, aa:e2:c9:59:3c:02)
//	-> Print(ACTIVE $print)
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
	-> ToDevice(ens3f0)
