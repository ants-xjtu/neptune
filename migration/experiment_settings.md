## intro

The libraries under `lib/` is now a complete mess.
Since another deadline is around the corner, 
I choose to let it go (again!).

However, to slightly improve the situation,
I feel obligated to record how those new libraries are generated
both for a reliable result and future self to conduct the experiments all over again.

I also prepare a 'snapshot' of `lib/` before I proceed,
and I will commit it if it's really necessary.

## rubik

This is rather easy.
`./libs/rubik.so` seems to be a clean copy outside every folder.
So I use `python batch-hash.py /home/hypermoon/neptune-yh/libs/rubik-final/rubik.so 10 ./libs/rubik.so`
to generate the new copies.
Another interesting proof that I'm on the right track is that
by running `readelf -d ./libs/rubik-final*`, 
you will find that every copy of hashed rubik share the same file size.

## nDPI

Similarly, `./libs/ndpiReader.so` is the taint-proof copy (how I wish I created this for every NF!).
So we can create the hashed copy and verify them easily.

## libnids

Things are getting a little complicated on Libnids...
As `bash_history` indicated, all `Libnids` copies are replaced by `./build/libMoon_Libnids_Slow`,
which I can't really remember the reason.
For now, I decide to trust the history.

As for the previous (I mean at Sep. 16, 2021) situation,
`Libnids`, `Libnids2` and `Libnids-slow` are identical.
And `Libnids-old` indicate the old (around August 2021) recipe (from `./build/libMoon_Libnids.so`),
as shown in its directory.

Weird enough, we don't include `Moon/LibnidsSlow.c` in the source tree.
I need more investigation into the chat about the motivation of replace `Moon/LibnidsWtf.c` with this.
For now, I use `python batch-hash.py /home/hypermoon/neptune-yh/libs/Libnids/forward.so 10 ./forward.so`
Note that I did not use `lib/forward.so`, but compile a fresh copy and use `cp ./build/libMoon_Libnids_Slow.so forward.so`,
because I really don't know the source of `lib/forward.so`.

*update:* it seems that verify via file size is not always correct.
A counterexample is though `libs/Libnids3/forward.so` and `libs/Libnids4/forward.so` are of the same size,
`readelf -s` will show different symbol tables.
(and as a result, `pcap_inject` in previous version is failing tiangou)
Therefore, I decide to use a clean copy of freshly compiled libnids,
compiled from `moon/Libnids_Slow.c`.

## fastclick

FastClick is the most complicated NF among this list.
Yet fortunately, I left the recipe at trello.

One thing to note: the version under `~/fastclick` is the one after nfd experiments,
which means it would support more elements than the previous (as per `./libs/fastclick9`, Sep. 2, 2021) version.
After recompiling fastclick and add `-shared -fPIC` in the makefile under `userlevel/`,
I successfully craft the identical version of `./libs/fastclick-final`, under `./libs/fastclick9` and `./libs/fastclick10`.
This means that though fastclick1~8 and 9~10 are different, they are functionally the same.

## Command

As `./RunMoon/go.sh` is coupled with one SFC on one chain model, 
I have to craft the command manually, just like the times before this script is created.

```
sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hypermoon/neptune-yh/libs/fastclick:/home/hypermoon/neptune-yh/libs/Libnids:/home/hypermoon/neptune-yh/libs/ndpi:/home/hypermoon/neptune-yh/libs/rubik-final:/home/hypermoon/neptune-yh/libs/fastclick2:/home/hypermoon/neptune-yh/libs/Libnids2:/home/hypermoon/neptune-yh/libs/ndpi2:/home/hypermoon/neptune-yh/libs/rubik-final2:/home/hypermoon/neptune-yh/libs/fastclick3:/home/hypermoon/neptune-yh/libs/Libnids3:/home/hypermoon/neptune-yh/libs/ndpi3:/home/hypermoon/neptune-yh/libs/rubik-final3:/home/hypermoon/neptune-yh/libs/fastclick4:/home/hypermoon/neptune-yh/libs/Libnids4:/home/hypermoon/neptune-yh/libs/ndpi4:/home/hypermoon/neptune-yh/libs/rubik-final4:/home/hypermoon/neptune-yh/libs/fastclick5:/home/hypermoon/neptune-yh/libs/Libnids5:/home/hypermoon/neptune-yh/libs/ndpi5:/home/hypermoon/neptune-yh/libs/rubik-final5:/home/hypermoon/neptune-yh/libs/fastclick6:/home/hypermoon/neptune-yh/libs/Libnids6:/home/hypermoon/neptune-yh/libs/ndpi6:/home/hypermoon/neptune-yh/libs/rubik-final6:/home/hypermoon/neptune-yh/libs/fastclick7:/home/hypermoon/neptune-yh/libs/Libnids7:/home/hypermoon/neptune-yh/libs/ndpi7:/home/hypermoon/neptune-yh/libs/rubik-final7:/home/hypermoon/neptune-yh/libs/fastclick8:/home/hypermoon/neptune-yh/libs/Libnids8:/home/hypermoon/neptune-yh/libs/ndpi8:/home/hypermoon/neptune-yh/libs/rubik-final8:/home/hypermoon/neptune-yh/libs/fastclick9:/home/hypermoon/neptune-yh/libs/Libnids9:/home/hypermoon/neptune-yh/libs/ndpi9:/home/hypermoon/neptune-yh/libs/rubik-final9:/home/hypermoon/neptune-yh/libs/fastclick10:/home/hypermoon/neptune-yh/libs/Libnids10:/home/hypermoon/neptune-yh/libs/ndpi10:/home/hypermoon/neptune-yh/libs/rubik-final10:/home/hypermoon/neptune-yh/build ./build/RunMoon -c 0xb -- ./build/libTianGou.so --pku 3 4 5 7
```

To test fastclick using only 5 flows:
```
sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hypermoon/neptune-yh/libs/fastclick:/home/hypermoon/neptune-yh/libs/fastclick2:/home/hypermoon/neptune-yh/libs/fastclick3:/home/hypermoon/neptune-yh/libs/fastclick4:/home/hypermoon/neptune-yh/libs/fastclick5:/home/hypermoon/neptune-yh/build ./build/RunMoon -c 0xb -- ./build/libTianGou.so --pku 3
```

And 10 flows:
```
sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hypermoon/neptune-yh/libs/fastclick:/home/hypermoon/neptune-yh/libs/fastclick2:/home/hypermoon/neptune-yh/libs/fastclick3:/home/hypermoon/neptune-yh/libs/fastclick4:/home/hypermoon/neptune-yh/libs/fastclick5:/home/hypermoon/neptune-yh/libs/fastclick6:/home/hypermoon/neptune-yh/libs/fastclick7:/home/hypermoon/neptune-yh/libs/fastclick8:/home/hypermoon/neptune-yh/libs/fastclick9:/home/hypermoon/neptune-yh/libs/fastclick10:/home/hypermoon/neptune-yh/build ./build/RunMoon -c 0xb -- ./build/libTianGou.so --pku 3
```

And to test libnids with 10 flows:
```
sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hypermoon/neptune-yh/libs/Libnids-clean:/home/hypermoon/neptune-yh/libs/Libnids-clean2:/home/hypermoon/neptune-yh/libs/Libnids-clean3:/home/hypermoon/neptune-yh/libs/Libnids-clean4:/home/hypermoon/neptune-yh/libs/Libnids-clean5:/home/hypermoon/neptune-yh/libs/Libnids-clean6:/home/hypermoon/neptune-yh/libs/Libnids-clean7:/home/hypermoon/neptune-yh/libs/Libnids-clean8:/home/hypermoon/neptune-yh/libs/Libnids-clean9:/home/hypermoon/neptune-yh/libs/Libnids-clean10:/home/hypermoon/neptune-yh/build ./build/RunMoon -c 0xb -- ./build/libTianGou.so --pku 4
```