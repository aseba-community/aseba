# End-to-end testing of Aseba Playground

## Creating multi-node networks

The testing scenario comprises five simulated Thymio-II robots partitioned into three networks.

1. A solo node with a program loaded from an Aesl file
2. A two-node network with separate programs loaded from an external Aesl file 
3. A two-node network with separate programs specified from an Aesl network whose definition is inlined in the Playground

The programs are simple beacons, that broadcast an **alive** message with the id of the node evry three seconds. They can be tested by connecting **asebadump** to the network and watching for events.
```
for i in 01 02 13 23; do asebadump tcp:port=333$i | head -5; done
```

The Playground files provide two ways of building the same networks.
* *test-1-processes.playground* runs Aseba utilities as separate processes, to join the nodes with switches and to install the programs;
* *test-2-networks.playground* declares the networks using a proposed extension of the Playground XML definition.

The latter is meant to fail at the present time, because the proposed extension is not yet implemented.
